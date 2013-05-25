#include "pmm.h"
#include "vmm.h"
#include "video.h"

#define PRESENT 0x1
#define READWRITE 0x2

#define PAGE_DIRECTORY ((unsigned char*)0xFFC00000)

extern void _invalidate_page_table(unsigned ptbl);
extern uint32_t _page_directory;
static uint32_t* pdir=&_page_directory; // Identity mapping of 0-4mb is still active

bool invalidate_page_table(physptr_t* ptr)
{
	//__asm__ volatile("invlpg %0" : : "m"(*ptr) : "memory");
	return true;
}

void flush_page_directory(void)
{
	__asm__ volatile("mov eax, cr3\n\t"
					 "mov cr3, eax" : : :);
}

void setup_vmm(void)
{
	pdir[0]=0; // Remove identity mapping
	//flush_page_directory();
}

vptr_t* kalloc_page(vaddr_t to)
{
	physaddr_t pfaddr=(physaddr_t)kalloc_page_frame();
	unsigned pdi=to >> 22;
	printix(pfaddr);
	printk("\n");
	printix(pdi);
	printk("\n");
	if(pdir[pdi * 0x1000])
	{
		printk("Page table exists\n");
	}
	else
	{
		printk("Allocate new page table\n");
	}

	return NULL;
}
