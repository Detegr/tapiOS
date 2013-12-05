#ifndef _TAPIOS_PROCESS_H_
#define _TAPIOS_PROCESS_H_

#include <stdint.h>
#include "pmm.h"
#include "vmm.h"

typedef struct process
{
	uint32_t pid;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	page_directory* pdir;
	struct process* next;
} process;

#ifndef SHARED_PROCESS_VARIABLES
volatile process* current_process;
volatile process* process_list;
#define SHARED_PROCESS_VARIABLES
#endif

void setup_multitasking(void);
void switch_active_process(void);
int fork(void);
int getpid(void);

#endif
