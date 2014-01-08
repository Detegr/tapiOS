#include "scheduler.h"
#include "processtree.h"
#include "process.h"
#include <terminal/vga.h>

struct process *get_next_process(void)
{
	struct process *ret;
	struct process *cur=(struct process*)current_process;
	if(!cur->next) return (struct process*)process_list;
	ret=cur->next;
	while(ret && (ret->state & (waiting|finished))) ret=ret->next;
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
