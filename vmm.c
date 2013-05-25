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
	__asm__ volatile("mov eax, cr3;"
					 "mov cr3, eax;"
					 : : : "eax");
}

void setup_vmm(void)
{
	// Map the last page table to page directory itself
	pdir[1023]=(((uint32_t)pdir) - 0xC0000000) & 0xFFFFF000;
	pdir[1023] |= PRESENT|READWRITE;

	physptr_t* ext=kalloc_page_frame();
	pdir[769]=(physaddr_t)ext|PRESENT|READWRITE;
	unsigned int* exti=(unsigned int*)ext;
	for(unsigned i=0, j=0x400000; i<1024; i++, j+=0x1000)
	{// Map next 4mb of physical memory for kernel
		exti[i]=j|PRESENT|READWRITE;
	}
	pdir[0]=0; // Remove identity mapping
}

vptr_t* kalloc_page(vaddr_t to)
{
	if(to==0x0) return NULL; // Do not allow mapping 0x0
	physaddr_t pfaddr=(physaddr_t)kalloc_page_frame();
	unsigned pdi=to >> 22;
	if(pdir[pdi] & PRESENT)
	{
		printk("Page table exists\n");
	}
	else
	{
		printk("Allocate new page table\n");
	}

	return NULL;
}
