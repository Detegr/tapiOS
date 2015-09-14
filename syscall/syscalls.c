#include "syscalls.h"
#include <task/process.h>
#include <task/multitasking.h>
#include <terminal/vga.h>
#include <irq/irq.h>
#include <util/scancodes.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <task/processtree.h>
#include <task/scheduler.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/poll.h>
#include <drivers/rtl8139.h>
#include <dev/pci.h>
#include <network/netdev.h>
#include <sys/socket.h>
#include <network/tcp.h>
#include <network/arp.h>
#include <network/socket.h>
#include <util/list.h>
#include <util/random.h>

extern void _return_from_exec(void);
extern void _return_to_userspace(void);

int _exit(int code);
int _write(int fd, uint8_t* to, uint32_t size);
int _read(int fd, uint8_t* to, uint32_t size);
void* _sbrk(int32_t increment);
int _open(const char* path, int flags);
int _readdir(int dirfd, struct dirent *ret);
pid_t _waitpid(pid_t pid, int *status, int options);
int _exec(const char *path, char **const argv, char **const envp);
int _fstat(int fd, struct stat *buf);
int _getcwd(char *buf, size_t size);
int _chdir(char *path);
int _close(int fd);
int _dup2(int oldfd, int newfd);
int _fcntl(int fd, int cmd, int arg);
int _ioctl(int fd, int req, void *argp);
int _poll(struct pollfd *fds, nfds_t nfds, int timeout);
int _mkdir(const char* path, mode_t mode);
int _socket(int domain, int type, int protocol);
int _connect(int sock, const struct sockaddr *addr, int addr_len);
int _kill(pid_t pid, int sig);

typedef int(*syscall_ptr)();
syscall_ptr syscalls[]={
	&_exit, &_write, &_read, (syscall_ptr)&_sbrk,
	&_open, &_dup2, &_readdir,
	&fork, &_waitpid, &_exec, &_fcntl, &getpid, &_fstat,
	&_getcwd, &_chdir, &_close, &_ioctl, &_poll, &_mkdir, &_socket,
	&_connect, &_kill
};

int _exit(int code)
{
	current_process->state=finished;

	for(int i=0; i<FD_MAX; ++i)
	{
		if(current_process->fds[i] != NULL)
		{
			vfs_close(current_process->fds[i]);
			current_process->fds[i]=0;
		}
	}

	// TODO: Free environment

	kfree(current_process->program);
	kfree(current_process->pdir);
	kfree(current_process->esp0);

	reap_finished_processes();

	/* Wait for the process to be switched.
	 * after the switch, we never come back
	 * to this process anymore. */

	while(1) __asm__ volatile("sti; hlt");
	return code; // Never reached.
}

int _read(int fd, uint8_t* to, uint32_t count)
{
	struct file *f=current_process->fds[fd];
	if(!f) PANIC();
	return vfs_read(f, to, count);
}

int _write(int fd, uint8_t* to, uint32_t count)
{
	struct file *f=current_process->fds[fd];
	if(!f)
	{
		kprintf("FD %d does not exist!\n", fd);
		PANIC();
	}
	return vfs_write(f, to, count);
}

void* _sbrk(int32_t increment)
{
	if(increment<0) PANIC();
	if(increment==0) return (void*)current_process->brk;

	uint32_t oldbrk=current_process->brk;
	uint32_t newbrk=(current_process->brk+increment+7) & ~7;

	int pagediff=(newbrk/0x1000)-(oldbrk/0x1000);
	if(pagediff == 0)
	{
		current_process->brk=newbrk;
		return (void*)oldbrk;
	}
	else if(pagediff>0)
	{
		for(int i=1; i<=pagediff; ++i)
		{// Allocate 'pagediff' pages to the end of current brk
			kalloc_page(oldbrk + (i * 0x1000), false, true);
		}
	}
	else
	{
		for(int i=1; i<=-pagediff; ++i)
		{// Free 'pagediff' pages from the end of current brk
			kfree_page(oldbrk + (i * 0x1000));
		}
	}
	current_process->brk=newbrk;
	return (void*)oldbrk;
}

static struct inode *getdir_helper(const char *path)
{
	char *dirpath=strndup(path, PATH_MAX);
	char *dir_name=dirname(dirpath);
	if(strlen(dir_name) == 1 && dir_name[0] == '.') dir_name=(char*)current_process->cwd;
	struct inode *dir=vfs_search((struct inode*)root_fs, dir_name);
	kfree(dirpath);
	return dir;
}

int _open(const char* path, int flags)
{
	struct inode *inode, *dir;
	inode=dir=NULL;

	if(!root_fs) PANIC();
	if(strnlen(path, PATH_MAX) >= PATH_MAX) return -ENAMETOOLONG;
	if(path[0]=='/')
	{// Absolute path
		inode=vfs_search((struct inode*)root_fs, path);
	}
	else
	{// Relative path
		dir=getdir_helper(path);
		if(!dir) return -ENOENT;
		inode=vfs_search(dir, path);
	}
	if(inode && ((flags & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL))) return -EEXIST;
	if(!inode && (flags & O_CREAT))
	{
		if(!dir)
		{
			dir=getdir_helper(path);
			if(!dir) return -ENOENT;
		}
		inode=vfs_new_inode(dir, path, 0x8000);
	}
	if(!inode) return -ENOENT;
	int status;
	struct file *f=vfs_open(inode, &status, flags);
	if(!f) return status;
	return newfd(f);
}

int _close(int fd)
{
	struct file *f=current_process->fds[fd];
	if(!f)
	{
		return -EBADF;
	}
	vfs_close(f);
	if(f->refcount == 0) kfree(f);
	current_process->fds[fd]=0;
	return 0;
}

int _readdir(int dirfd, struct dirent *ret)
{
	struct dirent *de=vfs_readdir(dirfd);
	if(de)
	{
		memcpy(ret, de, sizeof(struct dirent));
		return 0;
	}
	else return -1;
}

pid_t _waitpid(pid_t pid, int *status, int options)
{
	if(pid < -1) return -1; /* gid == -pid, NYI */
	if(pid == 0) return -1; /* gid == pid, NYI */

	if(pid == -1)
	{//Wait for any child process
		volatile struct pnode const *pnode=find_process_from_process_tree((struct process*)current_process);
		if(!pnode->first_child) return -1;

		pid_t ret=pnode->first_child->process->pid;
		current_process->state=waiting;
		while(pnode->first_child) sched_yield();
		current_process->state=running;
		if(status) *status = 0;
		return ret;
	}
	if(pid > 0)
	{//Wait for child with process id 'pid'
		volatile struct pnode const *pnode=find_process_by_pid(pid);
		if(!pnode) return -1;
		current_process->state=waiting;
		while(pnode) sched_yield();
		current_process->state=running;
		if(status) *status = 0;
		return pid;
	}
	return -1;
}

int _exec(const char *path, char **const argv, char **const envp)
{
	int fd=_open(path, 0);
	if(fd<0) return fd;

	int argc=0;
	for(int i=0;; ++i, ++argc)
	{
		char *p=argv[i];
		if(!p) break;
	}

	page_directory *old_pdir=current_process->pdir;
	page_directory *pdir=new_page_directory_from(old_pdir);

	int envc=0;
	if(envp) for(char **ep=envp; *ep; ++ep, ++envc) {}

	// Old process stack is in old_pdir and will be freed later on
	change_pdir(pdir);
	current_process->pdir=pdir;
	current_process->user_stack=setup_process_stack();

	struct file *f=current_process->fds[fd];
	uint8_t *prog=kmalloc(f->inode->size);
	uint32_t total=0;
	uint32_t r=_read(fd, prog, f->inode->size);
	if(r != 0 && r < f->inode->size)
	{
		kprintf("Read %d, expected %d\n", r, f->inode->size);
		PANIC();
	}
	vaddr_t entry=init_elf_get_entry_point(prog);

	char **argv_copy=(char**)_sbrk(sizeof(char*) * (argc+1));
	for(int i=0; i<argc; ++i)
	{
		change_pdir(old_pdir);
		int len=strlen(argv[i])+1;
		char str[len];
		strncpy(str, argv[i], len);
		change_pdir(pdir);

		argv_copy[i]=(char*)_sbrk(len);
		strncpy(argv_copy[i], str, len);
	}
	argv_copy[argc]=NULL;

	char **envp_copy=NULL;
	if(envc>0)
	{
		envp_copy=(char**)_sbrk(sizeof(char*) * (envc+1));
		for(int i=0; i<envc; ++i)
		{
			change_pdir(old_pdir);
			int len=0;
			len=strlen(envp[i])+1;
			char str[len];
			strncpy(str, envp[i], len);
			change_pdir(pdir);

			envp_copy[i]=(char*)_sbrk(len);
			strncpy(envp_copy[i], str, len);
		}
		envp_copy[envc]=NULL;
	}
	kfree(old_pdir);

	current_process->envp=envp_copy;
	setcwd_dirname((struct process*)current_process, argv_copy[0]);

	vptr_t *stack_top=setup_usermode_stack(entry, argc, argv_copy, envp_copy, current_process->user_stack + STACK_SIZE);

	__asm__ volatile("mov esp, %0; jmp %1" :: "r"(stack_top), "r"((uint32_t)_return_to_userspace));

	return 0; // Never reached
}

int _fstat(int fd, struct stat *buf)
{
	struct file *f=current_process->fds[fd];
	if(!f)
	{
		return -EBADF;
	}
	return vfs_stat(f, buf);
}

int _getcwd(char *buf, size_t size)
{
	strncpy(buf, (const char*)current_process->cwd, size);
	return 0;
}

int _chdir(char *path)
{
	if(strnlen(path, PATH_MAX) == PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	int fd=_open(path, O_RDONLY);
	if(fd<0)
	{
		return -ENOENT;
	}
	struct stat st;
	if((_fstat(fd, &st)) < 0)
	{
		return -EIO;
	}
	_close(fd);
	if(!S_ISDIR(st.st_mode))
	{
		return -ENOTDIR;
	}
	setcwd((struct process*)current_process, path);
	return 0;
}

int _dup2(int oldfd, int newfd)
{
	struct file *f=current_process->fds[oldfd];
	if(!f) return -EBADF;
	if(oldfd==newfd) return newfd;
	_close(newfd);
	f->refcount++;
	current_process->fds[newfd]=f;
	return 0;
}

int _fcntl(int fd, int cmd, int arg)
{
	if(cmd==F_DUPFD)
	{
		if(fd<0 || fd>=FD_MAX) return -EINVAL;
		for(int i=arg; i<FD_MAX; ++i)
		{
			if(!current_process->fds[i])
			{
				int ret=_dup2(fd, i);
				if(ret<0) return ret;
				return i;
			}
		}
		return -EMFILE;
	}
	return -EINVAL;
}

int _ioctl(int fd, int req, void *argp)
{
	switch(req)
	{
		case TCGETATTR:
		case TCSETATTR:
		{
			struct termios *t=argp;
			// TODO: isatty check
			struct file *f=current_process->fds[fd];
			if(!f) return -EBADF;
			return vfs_ioctl(f, req, argp);
		}
	}
	return -EINVAL;
}

int _poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret=0;
	for(nfds_t i=0; i<nfds; ++i)
	{
		fds[i].revents=0;
		if(fds[i].fd < 0) continue;
		struct file *f=current_process->fds[fds[i].fd];
		ret += vfs_poll(f, fds[i].events, &fds[i].revents);
	}
	return ret;
}

int _mkdir(const char *path, mode_t mode)
{
	if(strnlen(path, PATH_MAX) == PATH_MAX) return -ENAMETOOLONG;
	struct inode *dir=getdir_helper(path);
	if(!dir) return -ENOENT;
	int fd=_open(path, mode);
	if(fd>=0)
	{
		_close(fd);
		return -EEXIST;
	}
	struct inode *newdir=vfs_new_inode(dir, path, 0x4000);
	if(!newdir) return -EPERM;
	return 0;
}

int _socket(int domain, int type, int protocol)
{
	if(!network_devices) return -EINVAL;
	int status;
	struct inode *inode=alloc_netdev_inode();
	struct socket *f=kmalloc(sizeof(struct socket));

 	/* Refcount of the socket will be set to 2. The OS will have one ref and the
	 * userspace program creating this socket will have the other ref.
	 * This is because a socket should not be freed from the OS when it is closed
	 * by the userspace program. */
	f->refcount=2;
	f->inode=inode;
	f->pos=0;
	f->rcvbuf=kmalloc(TCP_RCVBUF_SIZE);
	return newfd((struct file*)f);
}

int _connect(int sock, const struct sockaddr *addr, int addr_len)
{
	struct sockaddr_in *addrin=(struct sockaddr_in*)addr;
	struct file *f=current_process->fds[sock];
	struct socket* s=(struct socket*)f;
	if(!f || !f->inode) return -EBADF;
	struct network_device *dev=f->inode->device;
	if(!dev) return -ENOTSOCK;
	bool mac_found=false;
	uint8_t mac[6];
	uint32_t dest_ip=addrin->sin_addr;
	mac_found=arp_find_mac(dest_ip, mac);
	if(!mac_found)
	{
		const struct arp_packet p=arp_request(dev, dest_ip);
		dev->n_act->tx(f, &p, sizeof(struct arp_packet));
		while(!arp_find_mac(dest_ip, mac)) sched_yield();
	}
	struct tcp_packet p;
	struct tcp_opts opts={
		.SYN=1
	};
	tcp_build_packet(dev, mac, addrin, &p, &opts, NULL, 0);
	p.tcp_header.port_src=rand() % 65535;
	p.tcp_header.ack_no=0;
	p.tcp_header.checksum=tcp_checksum(&p, 0);
	bool exists=false;
	list_foreach(open_sockets, volatile struct socket, sock)
	{
		if(sock->saddr.sin_port == addrin->sin_port)
		{
			exists=true;
			break;
		}
	}
	if(!exists)
	{
		struct socket* os=(struct socket*)open_sockets;
		s->saddr=*addrin;
		s->state=CONNECTING;
		s->seq_no=p.tcp_header.seq_no;
		s->ack_no=p.tcp_header.ack_no;
		s->src_port=p.tcp_header.port_src;
		memcpy(s->dst_mac, mac, 6);
		if(open_sockets){list_add(os, s);}
		else open_sockets=s;
		s->list.next=NULL;
	}
	dev->n_act->tx(f, &p, sizeof(struct tcp_packet));
	while(s->state==CONNECTING) sched_yield();
	if(s->state==CONNECTED) return 0;
	_close(sock);
	return -ECONNREFUSED;
}

int _kill(pid_t pid, int sig)
{
	kprintf("kill(%d, %d)\n", pid, sig);
	return -EINVAL;
}

void syscall(void *v)
{
	struct registers *r=(struct registers *)&v;
	__asm__ volatile(
		"mov eax, [%0];"
		"push edx;"
		"push ecx;"
		"push ebx;"
		"call eax;"
		"pop ebx;"
		"pop ebx;"
		"pop ebx;"
		:: "r"(&syscalls[r->eax-1]));
}
