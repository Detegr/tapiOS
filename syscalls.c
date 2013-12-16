#include "syscalls.h"
#include "process.h"
#include "vga.h"
#include "irq.h"
#include "scancodes.h"

int _exit(int code);
int _write(int fd, uint8_t* to, uint32_t size);
int _read(int fd, uint8_t* from, uint32_t size);
void* _sbrk(int32_t increment);

typedef int(*syscall_ptr)();
syscall_ptr syscalls[]={&_exit, &_write, &_read, (syscall_ptr)&_sbrk};

int _exit(int code)
{
	volatile process* p=process_list;
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
	if(count==0) return 0;
	process* p=find_active_process();
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
			else if(c==CHAR_BACKSPACE) delete_last_char(row);
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

void syscall(void)
{
	__asm__ volatile(
		"mov eax, %0[eax * 4 - 4];"
		"push edx;"
		"push ecx;"
		"push ebx;"
		"call eax;"
		"pop ebx;"
		"pop ebx;"
		"pop ebx;"
		:: "m"(syscalls));
}
