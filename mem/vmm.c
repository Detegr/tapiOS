#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include <terminal/vga.h>

#define PRESENT 0x1
#define READWRITE 0x2
#define USERMODE 0x4

#define PAGE_DIRECTORY ((uint32_t*)0xFFC00000)

extern uint32_t kernel_end_addr;
extern void _invalidate_page_table(unsigned ptbl);
extern uint32_t _page_directory;
static uint32_t* pdir=&_page_directory;
static page_directory* kernel_pdir=0;

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

uint32_t get_page(vaddr_t addr)
{
	unsigned pdi=addr >> 22;
	unsigned pti=(addr & 0x003FFFFF) >> 12;
	unsigned pd_index=pdi*0x400 + pti;
	return PAGE_DIRECTORY[pd_index];
}

uint32_t vaddr_to_physaddr(vaddr_t vaddr)
{
	return get_page(vaddr) & 0xFFFFF000;
}

static uint32_t get_page_table_entry(uint32_t pdi, uint32_t pti)
{
	return (uint32_t)PAGE_DIRECTORY[pdi * 0x400 + pti];
}

void set_page_table_entry(uint16_t pdi, uint32_t pti, uint32_t flags)
{
	uint32_t pd_index=pdi * 0x400 + pti;
	PAGE_DIRECTORY[pd_index]=flags;
	invalidate_page((uint32_t)&PAGE_DIRECTORY[pd_index]);
}

page_directory* DEBUG_get_kernel_pdir(void)
{
	return kernel_pdir;
}

static bool page_table_exists(uint32_t pdi)
{
	return current_pdir->entries[pdi].flags & PRESENT;
}

static void new_page_table(unsigned pdi, bool kernel, bool readwrite)
{
	physaddr_t paddr=kalloc_page_frame();
	uint32_t flags=PRESENT;
	if(!kernel) flags|=USERMODE;
	if(readwrite) flags|=READWRITE;
	current_pdir->entries[pdi].as_uint32=paddr|flags;
	for(unsigned i=0; i<1024; i++)
	{// Zero out the page table
		set_page_table_entry(pdi, i, 0);
	}
}

static void clone_page_table(uint32_t i)
{
	uint32_t* src=(uint32_t*)0xFF400000;
	uint32_t* to=(uint32_t*)0xFF800000;
	for(int j=0; j<1024; ++j)
	{
		uint32_t pd_index=i * 0x400 + j;
		if(src[pd_index] != 0)
		{
			physaddr_t srcaddr=src[pd_index] & 0xFFFFF000;
			// Temporarily map the pages to 0xF0000000 and 0xF0001000
			vptr_t* srcp=kalloc_page_from(srcaddr, 0xF0000000, true, false);
			vptr_t* dstp=kalloc_page(0xF0001000, true, true);
			invalidate_page((vaddr_t)srcp);
			invalidate_page((vaddr_t)dstp);
			physaddr_t paddr=get_page((vaddr_t)dstp) & 0xFFFFF000;

			// Set physaddr of the copied page to copied page table
			to[pd_index]=paddr|(src[pd_index] & 0x00000FFF)|0x7;
			memcpy(dstp, srcp, 0x1000);

			// Unmap temporary pages
			set_page((vaddr_t)srcp, 0);
			set_page((vaddr_t)dstp, 0);
			invalidate_page((vaddr_t)srcp);
			invalidate_page((vaddr_t)dstp);
		}
		else to[pd_index]=0;
	}
}

page_directory* clone_page_directory_from(page_directory* src)
{
	page_directory* ret=kmalloc(sizeof(page_directory));
	memset(ret, 0, sizeof(page_directory));

	physaddr_t src_pdir_paddr=get_page((vaddr_t)src);
	physaddr_t ret_pdir_paddr=get_page((vaddr_t)ret) & 0xFFFFF000;

	if(!page_table_exists(1021)) new_page_table(1021, true, true);
	if(!page_table_exists(1022)) new_page_table(1022, true, true);

	current_pdir->entries[1021].as_uint32=src_pdir_paddr|PRESENT|READWRITE;
	current_pdir->entries[1022].as_uint32=ret_pdir_paddr|PRESENT|READWRITE;

	for(int i=0; i<1021; ++i)
	{
		if(src->entries[i].as_uint32 == 0) {ret->entries[i].as_uint32=0;continue;}

		// If source entry is in kernel pdir, use it as it is not going to change.
		if(src->entries[i].as_uint32 == kernel_pdir->entries[i].as_uint32)
		{
			ret->entries[i].as_uint32=kernel_pdir->entries[i].as_uint32;
		}
		else
		{
			physaddr_t paddr=kalloc_page_frame();
			ret->entries[i].as_uint32=paddr|PRESENT|READWRITE|USERMODE;
			clone_page_table(i);
		}
	}

	current_pdir->entries[1021].as_uint32=0;
	current_pdir->entries[1022].as_uint32=0;
	ret->entries[1023].as_uint32=ret_pdir_paddr|PRESENT|USERMODE;
	return ret;
}

void setup_vmm(void)
{
	// Map the last page table to page directory itself
	pdir[1023]=(((uint32_t)pdir) - 0xC0000000) & 0xFFFFF000;
	pdir[1023] |= PRESENT|READWRITE;
	invalidate_page((uint32_t)&PAGE_DIRECTORY[1023]);

	kernel_pdir=(page_directory*)pdir;
	current_pdir=kernel_pdir;

	// Remove identity mapping
	for(int i=0; i<1024; ++i)
	{
		set_page(i * 0x1000, 0);
	}
	pdir[0]=0;

	// Map the possible ramfs to 0xC0000000
	for(uint32_t i=0xC0000000; i<kernel_end_addr + 0xC0000000; i+=0x1000)
	{
		if(!(get_page(i) & PRESENT))
		{
			kalloc_page_from(i - 0xC0000000, i, false, false);
		}
	}
	change_pdir(clone_page_directory_from(kernel_pdir));
	print_startup_info("VMM", true);
}

vptr_t* kalloc_page_from(physaddr_t from, vaddr_t to, bool kernel, bool readwrite)
{
	if(!from) PANIC();

	to = to & 0xFFFFF000; // Align by page
	if(to==0x0) return NULL; // Do not allow mapping 0x0

	unsigned pdi=to >> 22;
	unsigned pti=(to & 0x003FFFFF) >> 12;

	if(!page_table_exists(pdi))
	{
		new_page_table(pdi, kernel, readwrite);
	}
	else if(get_page_table_entry(pdi, pti) & PRESENT)
	{
		kprintf("Warning: Double page allocation at %x\n", to);
		return NULL;
	}
	if(is_free_page(from))
	{
		kprintf("Trying to map physical memory address %x to %x\n", from, to);
		PANIC();
	}
	uint32_t flags=PRESENT;
	if(readwrite) flags|=READWRITE;
	if(!kernel) flags|=USERMODE;
	set_page_table_entry(pdi, pti, from|flags);
	return (vptr_t*)to;
}

vptr_t* kalloc_page(vaddr_t to, bool kernel, bool readwrite)
{
	to = to & 0xFFFFF000; // Align by page
	if(to==0x0) return NULL; // Do not allow mapping 0x0
	return kalloc_page_from(kalloc_page_frame(), to, kernel, readwrite);
}

void kfree_page(vaddr_t from)
{
	physaddr_t paddr=get_page(from) & 0xFFFFF000;
	kfree_page_frame(paddr);
	set_page(from, 0);
}

void change_pdir(page_directory* pdir)
{
	__asm__ volatile("mov eax, %0;"
					 "mov cr3, eax;"
					 :: "r"(get_page((vaddr_t)pdir) & 0xFFFFF000) : "eax");
	current_pdir=pdir;
}
