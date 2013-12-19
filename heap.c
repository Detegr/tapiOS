#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "vga.h"

extern uint32_t kernel_end_addr;
static uint32_t kheap_end=0;

void *_kmalloc(uint32_t size, bool kernel, bool readwrite)
{
	if(size==0) return NULL;
	if(!kheap_end) kheap_end=(0xC0000000|kernel_end_addr)+0x1000; // Kernel heap starts where kernel code ends

	int amount;
	if(size % 0x1000 == 0) amount=size/0x1000;
	else amount=(size/0x1000)+1;
	if(amount==0) amount=1;

	vptr_t* ret=NULL;
	for(int i=0; i<amount; ++i, kheap_end+=0x1000)
	{
		if(!ret) ret=kalloc_page(kheap_end, false, true);
		else kalloc_page(kheap_end, false, true);
	}
	return ret;
}

inline void *kmalloc(uint32_t size)
{
	return _kmalloc(size, false, true);
}
