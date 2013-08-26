#include "pmm.h"
#include "vmm.h"
#include "video.h"

#define PRESENT 0x1
#define READWRITE 0x2

#define PAGE_DIRECTORY ((unsigned int*)0xFFC00000)

extern void _invalidate_page_table(unsigned ptbl);
extern uint32_t _page_directory;
static uint32_t* pdir=&_page_directory; // Identity mapping of 0-4mb is still active

void flush_page_directory(void)
{
	__asm__ volatile("mov eax, cr3;"
					 "mov cr3, eax;"
					 : : : "eax");
}

void setup_vmm(void)
{
	// Map the last page table to page directory itself
	pdir[1023]=(((uint32_t)pdir) - 0xC0000000) & 0xFFFFF000;
	pdir[1023] |= PRESENT|READWRITE;

	physaddr_t ext=kalloc_page_frame();
	pdir[769]=ext|PRESENT|READWRITE;
	unsigned int* exti=(unsigned int*)ext;
	for(unsigned i=0, j=0x400000; i<1024; i++, j+=0x1000)
	{// Map next 4mb of physical memory for kernel
		exti[i]=j|PRESENT|READWRITE;
	}
	pdir[0]=0; // Remove identity mapping
}

void new_page_table(unsigned pdi)
{
	physaddr_t paddr=kalloc_page_frame();
	pdir[pdi]=paddr|PRESENT|READWRITE;
	for(unsigned i=0; i<1024; i++)
	{// Zero out the page table
		PAGE_DIRECTORY[pdi * 0x400 + i]=0|PRESENT|READWRITE;
	}
}

vptr_t* kalloc_page(vaddr_t to)
{
	to = to & 0xFFFFF000; // Align by page
	if(to==0x0) return NULL; // Do not allow mapping 0x0
	physaddr_t pfaddr=kalloc_page_frame();
	unsigned pdi=to >> 22;
	unsigned pti=(to & 0x003FFFFF) >> 12;

	if(!(pdir[pdi] & PRESENT)) new_page_table(pdi);
	PAGE_DIRECTORY[pdi * 0x400 + pti]=pfaddr | PRESENT | READWRITE;
	return (vptr_t*)to;
}
