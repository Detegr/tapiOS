#include "multitasking.h"
#include "process.h"
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <mem/kmalloc.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <task/tss.h>

extern void _return_to_userspace(void);
extern uint32_t stack;
static uint32_t* stack_end_ptr=&stack;

void setup_tasking(void)
{
	process_list=kmalloc(sizeof(struct process));
	current_process=process_list;
	memset((struct process*)current_process, 0, sizeof(struct process));

	print_startup_info("Tasking system", true);
}

vptr_t *setup_process_stack(void)
{
	vptr_t *stack=NULL;
	for(uint32_t i=0xB0000000; i<=0xB0004000; i+=0x1000)
	{
		if(!stack) stack=kalloc_page(i, false, true);
		else kalloc_page(i, false, true);
	}
	return stack;
}

void free_process_stack(vptr_t *stack)
{
	for(uint32_t i=0xB0000000; i<=0xB0004000; i+=0x1000)
	{
		kfree_page(i);
	}
}

void setup_initial_process(vaddr_t entry_point)
{
	vptr_t *stack=setup_process_stack();
	vptr_t *stack_top=setup_usermode_stack(entry_point, 0, NULL, NULL, stack + STACK_SIZE);

	__asm__ volatile("cli");
	current_process->user_stack=stack;
	current_process->pdir=(page_directory*)initial_pdir;
	current_process->esp0=kmalloc(KERNEL_STACK_SIZE);
	current_process->pid=1;
	current_process->kesp=current_process->esp0+KERNEL_STACK_SIZE;
	current_process->state=running;
	current_process->active=true;
	strncpy((char*)current_process->cwd, "/", 2);
	memset(current_process->esp0, 0, KERNEL_STACK_SIZE);
	memset((void*)current_process->fds, 0, FD_MAX*sizeof(struct file*));
	tss.esp0=(uint32_t)current_process->kesp;

	__asm__ volatile(
		"mov esp, %0;"
		"jmp %1;" :: "r"(stack_top),"r"(_return_to_userspace));
}

