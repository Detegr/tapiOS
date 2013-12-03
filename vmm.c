#include "pmm.h"
#include "vmm.h"
#include "video.h"

#define PRESENT 0x1
#define READWRITE 0x2

#define PAGE_DIRECTORY ((uint32_t*)0xFFC00000)

extern void _invalidate_page_table(unsigned ptbl);
extern uint32_t _page_directory;
static uint32_t* pdir=&_page_directory;

static inline void flush_page_directory(void)
{
	__asm__ volatile("mov eax, cr3;"
					 "mov cr3, eax;"
					 : : : "eax");
}

static inline void invalidate_page(uint32_t addr)
{
	__asm__ volatile("invlpg [%0]" :: "r"(addr) : "memory");
}

void set_page(vaddr_t addr, uint32_t flags)
{
	unsigned pdi=addr >> 22;
	unsigned pti=(addr & 0x003FFFFF) >> 12;
	unsigned pd_index=pdi*0x400 + pti;
	PAGE_DIRECTORY[pd_index]=flags;
	invalidate_page((uint32_t)&PAGE_DIRECTORY[pd_index]);
}

uint32_t get_page_table_entry(uint16_t pdi, uint32_t pti)
{
	return (uint32_t)PAGE_DIRECTORY[pdi * 0x400 + pti];
}

void set_page_table_entry(uint16_t pdi, uint32_t pti, uint32_t flags)
{
	uint32_t pd_index=pdi * 0x400 + pti;
	PAGE_DIRECTORY[pd_index]=flags;
	invalidate_page((uint32_t)&PAGE_DIRECTORY[pd_index]);
}

void setup_vmm(void)
{
	// Map the last page table to page directory itself
	pdir[1023]=(((uint32_t)pdir) - 0xC0000000) & 0xFFFFF000;
	pdir[1023] |= PRESENT|READWRITE;
	invalidate_page((uint32_t)&PAGE_DIRECTORY[1023]);

	// Remove identity mapping
	for(int i=0; i<1024; ++i)
	{
		set_page(i * 0x1000, 0);
	}
	pdir[0]=0;
}

void new_page_table(unsigned pdi, uint32_t to)
{
	physaddr_t paddr=kalloc_page_frame();
	pdir[pdi]=paddr|PRESENT|READWRITE;
	for(unsigned i=0; i<1024; i++)
	{// Zero out the page table
		set_page_table_entry(pdi, i, 0);
	}
}

vptr_t* kalloc_page(vaddr_t to)
{
	to = to & 0xFFFFF000; // Align by page
	if(to==0x0) return NULL; // Do not allow mapping 0x0
	physaddr_t pfaddr=kalloc_page_frame();
	unsigned pdi=to >> 22;
	unsigned pti=(to & 0x003FFFFF) >> 12;

	if(!(pdir[pdi] & PRESENT))
	{
		new_page_table(pdi, to);
	}
	else if(PAGE_DIRECTORY[pdi * 0x400 + pti] & PRESENT)
	{
		printk("Warning: Double page allocation at ");
		printix(to);
		printk("\n");
	}
	set_page_table_entry(pdi, pti, pfaddr|PRESENT|READWRITE);
	return (vptr_t*)to;
}
