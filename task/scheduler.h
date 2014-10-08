#ifndef _TAPIOS_SCHEDULER_H_
#define _TAPIOS_SCHEDULER_H_

struct process *get_next_process(void);
void reap_finished_processes(void);
void sched_yield(void);
void switch_task(void);

#endif
