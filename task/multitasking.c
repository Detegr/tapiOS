#include "multitasking.h"
#include "process.h"
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <task/tss.h>

extern void _return_to_userspace(void);
extern uint32_t stack;
static uint32_t* stack_end_ptr=&stack;

#define STACK_SIZE 0x4000 // 16kB

void setup_tasking(void)
{
	process_list=kmalloc(sizeof(struct process));
	current_process=process_list;
	memset((struct process*)current_process, 0, sizeof(struct process));

	print_startup_info("Tasking system", true);
}

void setup_initial_process(vaddr_t entry_point)
{
	vptr_t *stack=NULL;
	for(uint32_t i=0xB0000000; i<=0xB0004000; i+=0x1000)
	{
		if(!stack) stack=kalloc_page(i, false, true);
		else kalloc_page(i, false, true);
	}

	vptr_t *stack_top=setup_usermode_stack(entry_point, stack + STACK_SIZE);

	__asm__ volatile("cli");
	current_process->user_stack=stack;
	current_process->pdir=current_pdir;
	current_process->esp0=kmalloc(KERNEL_STACK_SIZE);
	current_process->pid=1;
	current_process->kesp=current_process->esp0+KERNEL_STACK_SIZE;
	current_process->state=running;
	current_process->active=true;
	memset(current_process->esp0, 0, KERNEL_STACK_SIZE);
	tss.esp0=(uint32_t)current_process->kesp;

	__asm__ volatile(
		"mov esp, %0;"
		"jmp %1;" :: "r"(stack_top),"r"(_return_to_userspace));
}

