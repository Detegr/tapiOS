#include "syscalls.h"
#include "vga.h"

void syscall(void)
{
	int call, ebx, ecx, edx;
	__asm__ volatile("mov %0, eax;" : "=r"(call) :: "eax");
	__asm__ volatile("mov %0, ebx;" : "=r"(ebx)  :: "ebx");
	__asm__ volatile("mov %0, ecx;" : "=r"(ecx)  :: "ecx");
	__asm__ volatile("mov %0, edx;" : "=r"(edx)  :: "edx");
	switch(call)
	{
		case WRITE:
		{
			char* str=(char*)ebx;
			for(int i=0; i<ecx; ++i)
			{
				kprintc(str[i]);
			}
		}
		default:
			break;
	}
}
