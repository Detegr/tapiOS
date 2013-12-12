#include "syscalls.h"
#include "process.h"
#include "vga.h"
#include "irq.h"
#include "scancodes.h"

int syscall(void)
{
	int call, ebx, ecx, edx;
	__asm__ volatile("mov %0, eax;" : "=r"(call) :: "eax");
	__asm__ volatile("mov %0, ebx;" : "=r"(ebx)  :: "ebx");
	__asm__ volatile("mov %0, ecx;" : "=r"(ecx)  :: "ecx");
	__asm__ volatile("mov %0, edx;" : "=r"(edx)  :: "edx");
	switch(call)
	{
		case EXIT:
		{
			volatile process* p=process_list;
			if(process_list == current_process)
			{
				process_list=process_list->next;
				__asm__ volatile("sti; hlt");
				break;
			}
			while(true)
			{
				if(p->next==current_process)
				{
					p->next=current_process->next;
					__asm__ volatile("sti; hlt");
					break;
				}
				p=p->next;
			}
			break;
		}
		case WRITE:
		{
			char* str=(char*)ebx;
			for(int i=0; i<ecx; ++i)
			{
				kprintc(str[i]);
			}
			break;
		}
		case READ:
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
			uint8_t* buf=(uint8_t*)ebx;
			int j=0;
			for(int i=0; i<p->keyp-1; ++i)
			{
				char c=char_for_scancode(p->keybuf[i]);
				if(c==CHAR_UNHANDLED || c==CHAR_UP) continue;
				else if(c=='\n') break;
				else if(c==CHAR_BACKSPACE)
				{
					if(j==0) continue;
					else if(j<0) PANIC();
					buf[j--]=0;
				}
				else buf[j++]=c;
			}
			return j;
		}
		default:
			break;
	}
	return 0;
}
