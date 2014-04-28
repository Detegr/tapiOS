#include "syscalls.h"
#include <task/process.h>
#include <task/multitasking.h>
#include <terminal/vga.h>
#include <irq/irq.h>
#include <util/scancodes.h>
#include <task/process.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <task/processtree.h>
#include <task/scheduler.h>
#include <sys/fcntl.h>

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

typedef int(*syscall_ptr)();
syscall_ptr syscalls[]={
	&_exit, &_write, &_read, (syscall_ptr)&_sbrk,
	&_open, &_dup2, &_readdir,
	&fork, &_waitpid, &_exec, NULL, &getpid, &_fstat,
	&_getcwd, &_chdir, &_close
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
	if(fd>2)
	{
		struct file *f=current_process->fds[fd];
		if(!f) PANIC();
		return vfs_read(f, to, count);
	}
	if(count==0) return 0;
	struct process* p=find_active_process();
	if(!p) PANIC();
	memset(p->keybuf, 0, 256);
	uint32_t bufptr=0;
	for(int i=0; i<256; ++i)
	{
		if(p->stdoutbuf[i] == 0)
		{
			bufptr=i;
			break;
		}
	}
	uint32_t j=0;
	if(bufptr>0)
	{
		if(bufptr>count)
		{
			memcpy(to, p->stdoutbuf, count);
			memmove(p->stdoutbuf, p->stdoutbuf+count, 256-count);
			return count;
		}
		else
		{
			memcpy(to, p->stdoutbuf, count);
			memset(p->stdoutbuf, 0, 256);
			return count;
		}
	}
	int prevkp=p->keyp;
	int row=get_cursor_row();
	int col=get_cursor_col();
	update_cursor();
	while(true)
	{
		prevkp=p->keyp;
		__asm__ volatile("sti;hlt");
		if(p->keyp-prevkp == 0) continue;
		for(int i=prevkp; i<=p->keyp; ++i)
		{
			char c=char_for_scancode(p->keybuf[i]);
			if(c==CHAR_UP || c==CHAR_UNHANDLED) continue;
			else if(c==CHAR_BACKSPACE) delete_last_char(row, col);
			else kprintc(c);
			update_cursor();
			if(c=='\n') goto done;
		}
	}
done:
	hide_cursor();
	int i=0;
	for(i=0; i<p->keyp; ++i)
	{
		char c=char_for_scancode(p->keybuf[i]);
		if(c==CHAR_UNHANDLED || c==CHAR_UP) continue;
		else if(c=='\n')
		{
			p->stdoutbuf[bufptr++]=c;
			break;
		}
		else if(c==CHAR_BACKSPACE)
		{
			if(bufptr==0) continue;
			p->stdoutbuf[bufptr--]=0;
		}
		else
		{
			p->stdoutbuf[bufptr++]=c;
		}
	}
	for(j=0; j<bufptr; ++j)
	{
		if(j==count) break;
		to[j]=p->stdoutbuf[j];
	}
	memmove(p->keybuf, p->keybuf+i, 256-i);
	p->keyp-=i;
	memset(p->keybuf+i, 0, 256-i);
	memmove(p->stdoutbuf, p->stdoutbuf+j, 256-j);
	return j;
}

int _write(int fd, uint8_t* to, uint32_t size)
{
	if(fd<=2)
	{
		char* str=(char*)to;
		for(uint32_t i=0; i<size; ++i)
		{
			kprintc(str[i]);
		}
		return size;
	}
	else
	{
		return -EBADF;
	}
}

void* _sbrk(int32_t increment)
{
	if(increment==0) return (void*)current_process->brk;

	uint32_t newbrk=current_process->brk+increment;
	int diff=newbrk-current_process->brk;
	if(diff == 0) return (void*)current_process->brk;

	int pagediff=(newbrk/0x1000)-(current_process->brk/0x1000);
	if(pagediff == 0) return (void*)current_process->brk;

	if(diff>0)
	{
		for(int i=1; i<=pagediff; ++i)
		{// Allocate 'pagediff' pages to the end of current brk
			kalloc_page((current_process->brk + (i * 0x1000)) & 0xFFFFF000, false, true);
		}
	}
	else
	{
		for(int i=1; i<=pagediff; ++i)
		{// Free 'pagediff' pages from the end of current brk
			kfree_page(current_process->brk + (i * 0x1000));
		}
	}
	uint32_t oldbrk=current_process->brk;
	current_process->brk=newbrk;
	return (void*)oldbrk;
}
inline void* sbrk(int32_t increment)
{
	return _sbrk(increment);
}

int _open(const char* path, int flags)
{
	// flags ignored for now, expecting O_RDONLY
	// also expecting a full path
	if(!root_fs) PANIC();
	if(strnlen(path, PATH_MAX) >= PATH_MAX) return -ENAMETOOLONG;
	struct inode *inode=vfs_search((struct inode*)root_fs, path);
	if(inode && (flags & (O_CREAT|O_EXCL))) return -EEXIST;
	int status;
	if(!inode && (flags & O_CREAT)) inode=vfs_new_inode((struct inode*)root_fs, path);
	if(!inode) return -ENOENT;
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
	else
	{
		return -1;
	}
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
		while(pnode->first_child) __asm__("sti;hlt");
		current_process->state=running;
		if(status) *status = 0;
		return ret;
	}
	if(pid > 0)
	{//Wait for child with process id 'pid'
		volatile struct pnode const *pnode=find_process_by_pid(pid);
		if(!pnode) return -1;
		current_process->state=waiting;
		while(pnode) __asm__("sti;hlt");
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

	// TODO: This current_process->brk fiddling is ugly.
	char **argv_copy=(char**)current_process->brk;
	for(int i=0; i<argc; ++i)
	{
		change_pdir(old_pdir);
		int len=strlen(argv[i])+1;
		char str[len];
		memcpy(str, argv[i], len);
		change_pdir(pdir);

		// TODO: Calculate this in a reasonable way...
		current_process->brk+=0x1000;
		argv_copy[i]=(char*)kalloc_page(current_process->brk, false, true);

		memcpy(argv_copy[i], str, len);
	}
	kfree(old_pdir);

	setcwd_dirname((struct process*)current_process, argv_copy[0]);
	argv_copy[argc]=NULL;
	current_process->brk+=0x1000;
	kalloc_page(current_process->brk, false, true);

	vptr_t *stack_top=setup_usermode_stack(entry, argc, argv_copy, current_process->user_stack + STACK_SIZE);

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
