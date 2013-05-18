#ifndef _TAPIOS_VMM_H_
#define _TAPIOS_VMM_H_

typedef unsigned char vptr_t;
typedef unsigned long vaddr_t;

vptr_t* kalloc_page(vaddr_t to);
void setup_vmm(void);

#endif
