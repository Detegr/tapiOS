#ifndef _TAPIOS_KMALLOC_H_
#define _TAPIOS_KMALLOC_H_

#include <util/util.h>

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif
