#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "video.h"

extern uint32_t kernel_end_addr;
static uint32_t kheap_end=0;

void* kmalloc(uint32_t size)
{
	if(!kheap_end) kheap_end=(0xC0000000|kernel_end_addr)+0x1000; // Kernel heap starts where kernel code ends
	int amount=size/0x1000;
	if(!amount) amount=1;
	vptr_t* ret=NULL;
	for(int i=0; i<amount; ++i, kheap_end+=0x1000)
	{
		if(!ret) ret=kalloc_page(kheap_end);
		else kalloc_page(kheap_end);
	}
	return ret;
}
