#include "syscalls.h"
#include "process.h"
#include "vga.h"
#include "irq.h"

void syscall(void)
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
				timer_handler(true);
				break;
			}
			while(true)
			{
				if(p->next==current_process)
				{
					p->next=current_process->next;
					timer_handler(true);
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
			update_cursor();
		}
		default:
			break;
	}
}
