#include "multitasking.h"
#include "process.h"
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <terminal/vga.h>
#include <util/util.h>

extern uint32_t stack;
static uint32_t* stack_end_ptr=&stack;

static void copy_kernel_stack(vaddr_t to, uint32_t size)
{
	for(uint32_t i=0x1000; i<=size; i+=0x1000)
	{
		kalloc_page(to-i, false, true);
	}
	vptr_t* toptr=(vptr_t*)to;

	uint32_t esp, ebp;
	__asm__ volatile("mov %0, esp;"
					 "mov %1, ebp;" : "=r"(esp),"=r"(ebp));

	vaddr_t stackend=(uint32_t)stack_end_ptr;
	vaddr_t stackstart=stackend+size;

	uint32_t offset=to-stackstart;
	uint32_t to_esp=esp+offset;
	uint32_t to_ebp=ebp+offset;

	memcpy(toptr-size, stack_end_ptr, size);

	vaddr_t toaddr=(vaddr_t)toptr;
	uint32_t* p=(uint32_t*)(toptr-size);
	for(uint32_t i=0; i<4096; i++) // 16k stack
	{
		if(p[i] >= stackend && p[i] <= stackstart)
		{
			p[i]+=offset;
		}
	}

	__asm__ volatile("mov esp, %0;"
					 "mov ebp, %1;" :: "r"(to_esp), "r"(to_ebp));
}

void setup_multitasking(void)
{
	__asm__ volatile("cli;");
	copy_kernel_stack(0xE0000000, 0x4000);

	process_list=kmalloc(sizeof(process));
	current_process=process_list;
	memset((void*)current_process, 0, sizeof(process));
	current_process->active=false;
	current_process->pid=0;
	current_process->pdir=current_pdir;
	current_process->esp0=kmalloc(KERNEL_STACK_SIZE);
	memset(current_process->esp0, 0, KERNEL_STACK_SIZE);
	__asm__ volatile("sti;");

	print_startup_info("Multitasking", true);
}

