#include "syscalls.h"
#include <task/process.h>
#include <task/multitasking.h>
#include <terminal/vga.h>
#include <irq/irq.h>
#include <util/scancodes.h>
#include <task/process.h>
#include <mem/heap.h>
#include <fs/vfs.h>

extern void _return_from_exec(void);
extern void _return_to_userspace(void);

int _exit(int code);
int _write(int fd, uint8_t* to, uint32_t size);
int _read(int fd, uint8_t* to, uint32_t size);
void* _sbrk(int32_t increment);
int _open(const char* path, int flags);
struct DIR *_opendir(const char *dirpath);
int _readdir(DIR *dirp, struct dirent *ret);
int _wait(int *status);
int _exec(const char *path, char **const argv, char **const envp);

typedef int(*syscall_ptr)();
syscall_ptr syscalls[]={
	&_exit, &_write, &_read, (syscall_ptr)&_sbrk,
	&_open, (syscall_ptr)&_opendir, &_readdir,
	&fork, &_wait, &_exec
};

int _exit(int code)
{
	volatile struct process* p=process_list;
	if(process_list == current_process)
	{
		process_list=process_list->next;
		__asm__ volatile("sti; hlt");
		return code;
	}
	while(true)
	{
		if(p->next==current_process)
		{
			p->next=current_process->next;
			__asm__ volatile("sti; hlt");
			return code;
		}
		p=p->next;
	}
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
		if(p->keyp==0) continue;
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
	char* str=(char*)to;
	for(unsigned i=0; i<size; ++i)
	{
		kprintc(str[i]);
	}
	return size;
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

int _open(const char* path, int flags)
{
	// flags ignored for now, expecting O_RDONLY
	// also expecting a full path
	if(!root_fs) PANIC();
	struct inode *inode=vfs_search((struct inode*)root_fs, path);
	if(inode)
	{
		struct file *f=kmalloc(sizeof(struct file)); // TODO: Free
		if(vfs_open(inode, f) < 0) return -1;
		return newfd(f);
	}
	else return -1;
}

struct DIR *_opendir(const char *dirpath)
{
	if(!root_fs) PANIC();
	struct inode *inode=vfs_search((struct inode*)root_fs, dirpath);
	if(inode)
	{
		if(!(inode->flags & 0x4000)) // TODO: Maybe not use ext2 stuff for flags?
		{
			//errno=ENODIR;
			return NULL;
		}
		else
		{
			struct file *f=kmalloc(sizeof(struct file)); // TODO: Free
			if(vfs_open(inode, f) < 0) return NULL;
			DIR *ret=kmalloc(sizeof(struct DIR));
			ret->dir_fd=newfd(f);
			return ret;
		}
	}
	else
	{
		//errno=ENOENT;
		return NULL;
	}
}

int _readdir(DIR *dirp, struct dirent *ret)
{
	struct dirent *de=vfs_readdir(dirp);
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

int _wait(int *status)
{
	return -1;
}

int _exec(const char *path, char **const argv, char **const envp)
{
	int fd=_open(path, 0);
	if(fd<0) return -1;

	int argc=0;
	for(int i=0;; ++i, ++argc)
	{
		char *p=argv[i];
		if(!p) break;
	}

	page_directory *old_pdir=current_process->pdir;
	page_directory *pdir=new_page_directory_from(old_pdir);

	// TODO: Free old pdir and old stack
	change_pdir(pdir);
	current_process->pdir=pdir;
	current_process->user_stack=setup_process_stack();

	struct file *f=current_process->fds[fd];
	uint8_t *prog=kmalloc(f->inode->size);
	uint32_t r=_read(fd, prog, f->inode->size);
	if(!(r==f->inode->size || r==0)) PANIC();

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
	argv_copy[argc]=NULL;
	current_process->brk+=0x1000;
	kalloc_page(current_process->brk, false, true);

	vptr_t *stack_top=setup_usermode_stack(entry, argc, argv_copy, current_process->user_stack + STACK_SIZE);

	__asm__ volatile("mov esp, %0; jmp %1" :: "r"(stack_top), "r"((uint32_t)_return_to_userspace));

	return 0; // Never reached
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
