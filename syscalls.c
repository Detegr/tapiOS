#include "syscalls.h"
#include "process.h"
#include "vga.h"
#include "irq.h"
#include "scancodes.h"

int _exit(int code);
int _write(int fd, uint8_t* to, uint32_t size);
int _read(int fd, uint8_t* from, uint32_t size);

typedef int(*syscall_ptr)();
syscall_ptr syscalls[]={&_exit, &_write, &_read};

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

int _read(int fd, uint8_t* to, uint32_t size)
{
	process* p=find_active_process();
	if(!p) PANIC();
	memset(p->keybuf, 0, 256);
	p->keyp=0;
	int prevkp=p->keyp;
	int row=get_cursor_row();
	update_cursor();
	while(true)
	{
		prevkp=p->keyp;
		__asm__ volatile("sti;hlt");
		if(p->keyp==0) continue;
		for(int i=prevkp; i<=p->keyp-1; ++i)
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
	uint32_t j=0;
	for(int i=0; i<p->keyp-1; ++i)
	{
		if(j==size) break;
		char c=char_for_scancode(p->keybuf[i]);
		if(c==CHAR_UNHANDLED || c==CHAR_UP) continue;
		else if(c=='\n') break;
		else if(c==CHAR_BACKSPACE)
		{
			if(j==0) continue;
			to[j--]=0;
		}
		else to[j++]=c;
	}
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
