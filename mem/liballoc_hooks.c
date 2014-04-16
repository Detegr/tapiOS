#include "liballoc.h"
#include "pmm.h"
#include "vmm.h"
#include <terminal/vga.h>

extern int kernel_end_addr;
static int kheap_end=0;

size_t liballoc_lock()
{
	__asm__ volatile("cli;");
	return 0;
}

size_t liballoc_unlock()
{
	__asm__ volatile("sti;");
	return 0;
}

void *liballoc_alloc(size_t pages)
{
	if(pages==0) return NULL;
	if(!kheap_end) kheap_end=((0xC0000000|kernel_end_addr)+0x1000) & 0xFFFFF000; // Kernel heap starts where kernel code ends

	vptr_t* ret=NULL;
	for(size_t i=0; i<pages; ++i, kheap_end+=0x1000)
	{
		if(!ret) ret=kalloc_page(kheap_end, false, true);
		else kalloc_page(kheap_end, false, true);
	}
	return ret;
}

size_t liballoc_free(void *ptr, size_t pages)
{
	vaddr_t addr=(vaddr_t)ptr;
	for(size_t i=0; i<pages; ++i, addr+=0x1000)
	{
		kfree_page(addr);
	}
	return 0;
}
