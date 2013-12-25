#ifndef _TAPIOS_MULTITASKING_H_
#define _TAPIOS_MULTITASKING_H_

#include <stdint.h>
#include <mem/vmm.h>

void setup_tasking(void);
void setup_initial_process(vaddr_t entry_point);

#endif
