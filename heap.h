#ifndef _TAPIOS_HEAP_H_
#define _TAPIOS_HEAP_H_

#include <stdint.h>
#include <stdbool.h>

void *_kmalloc(uint32_t size, bool kernel, bool readwrite);
void* kmalloc(uint32_t size);

#endif
