#include "scheduler.h"
#include "processtree.h"
#include "process.h"
#include <terminal/vga.h>
#include "tss.h"

struct process *get_next_process(void)
{
	struct process *ret;
	struct process *cur=(struct process*)current_process;
	if(!cur->next) return (struct process*)process_list;
	ret=cur->next;
	while(ret && (ret->state & finished)) ret=ret->next;
	if(!ret) return (struct process*)process_list;
	return ret;
}

void reap_finished_processes(void)
{
	volatile struct process *pp=NULL;
	volatile struct process *p=process_list;
	while(p)
	{
		if(p->state==finished)
		{
			if(pp) pp->next=p->next;
			delete_process_from_process_tree((const struct process*)p);
		}
		pp=p;
		p=p->next;
	}
}

void switch_task(void)
{
	if(!current_process) return;

	struct process *new_process=get_next_process();
	struct process *old_process=(struct process*)current_process;
	if(!new_process || old_process == new_process) return;

	current_process=new_process;
	physaddr_t pageaddr=get_page((vaddr_t)new_process->pdir) & 0xFFFFF000;
	tss.esp0=((vaddr_t)current_process->esp0)+KERNEL_STACK_SIZE;

	/* Save 8 general purpose registers, save old kernel stack pointer
	 * and switch kernel stack pointers. Also change the page directory to
	 * the switced process' page directory. */
	__asm__ volatile(
		"lea edx, 1f;"
		"push edx;"
		"pushad;"
		"lea edx, %0;"
		"mov [edx], esp;"
		"mov cr3, %2;"
		"mov esp, %1;"
		"popad;"
		"ret;"
		"1:" :: "m"(old_process->kesp), "b"(new_process->kesp), "c"(pageaddr));
}

void sched_yield(void)
{
	switch_task();
}
