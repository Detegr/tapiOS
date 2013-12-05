#include "pmm.h"
#include "vmm.h"
#include "vga.h"
#include "heap.h"

#define PRESENT 0x1
#define READWRITE 0x2

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
	return kernel_pdir->entries[pdi].flags & PRESENT;
}

static void new_page_table(unsigned pdi)
{
	physaddr_t paddr=kalloc_page_frame();
	kernel_pdir->entries[pdi].as_uint32=paddr|PRESENT|READWRITE;
	for(unsigned i=0; i<1024; i++)
	{// Zero out the page table
		set_page_table_entry(pdi, i, 0);
	}
}

static void clone_page_table(uint32_t i)
{
	for(int j=0; j<1021; ++j)
	{
		if(get_page_table_entry(1021, j) != 0)
		{
			kprintf("Copying %x -> %x, index: %d\n", get_page_table_entry(1021, j), get_page_table_entry(1022, j), j);
			set_page_table_entry(1022, j, get_page_table_entry(1021, j));
		}
	}
}

page_directory* clone_page_directory_from(page_directory* src)
{
	page_directory* ret=kmalloc(sizeof(page_directory));
	kprintf("pdir size: %d, pdir: %x, entries addr: %x\n", sizeof(page_directory), ret, ret->entries);

	physaddr_t src_pdir_paddr=get_page((vaddr_t)src);
	physaddr_t ret_pdir_paddr=get_page((vaddr_t)ret);

	if(!page_table_exists(1021)) new_page_table(1021);
	if(!page_table_exists(1022)) new_page_table(1022);

	kernel_pdir->entries[1021].as_uint32=src_pdir_paddr|PRESENT|READWRITE;
	kernel_pdir->entries[1022].as_uint32=ret_pdir_paddr|PRESENT|READWRITE;
	flush_page_directory();

	for(int i=0; i<1024; ++i)
	{
		// If source entry is in kernel pdir, use it as it is not going to change.
		//if(src->entries[i].as_uint32 == kernel_pdir->entries[i].as_uint32) ret->entries[i].as_uint32=kernel_pdir->entries[i].as_uint32;
		//else
		{
			if(src->entries[i].as_uint32 == 0) ret->entries[i].as_uint32=0;
			else
			{
				// Map ret's page dir to the end of the address space
				clone_page_table(i);
				ret->entries[i].flags=src->entries[i].flags;
				ret->entries[i].addr=(ret_pdir_paddr >> 12);
			}
		}
	}

	kernel_pdir->entries[1021].as_uint32=0;
	kernel_pdir->entries[1022].as_uint32=0;
	flush_page_directory();
	return ret;
}

void setup_vmm(void)
{
	// Map the last page table to page directory itself
	pdir[1023]=(((uint32_t)pdir) - 0xC0000000) & 0xFFFFF000;
	pdir[1023] |= PRESENT|READWRITE;
	invalidate_page((uint32_t)&PAGE_DIRECTORY[1023]);

	kernel_pdir=(page_directory*)pdir;

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
			kalloc_page_from(i - 0xC0000000, i);
		}
	}
	print_startup_info("VMM", "OK\n");
}

vptr_t* kalloc_page_from(physaddr_t from, vaddr_t to)
{
	if(!from) PANIC();

	to = to & 0xFFFFF000; // Align by page
	if(to==0x0) return NULL; // Do not allow mapping 0x0

	unsigned pdi=to >> 22;
	unsigned pti=(to & 0x003FFFFF) >> 12;

	if(!page_table_exists(pdi))
	{
		new_page_table(pdi);
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
	set_page_table_entry(pdi, pti, from|PRESENT|READWRITE);
	return (vptr_t*)to;
}

vptr_t* kalloc_page(vaddr_t to)
{
	to = to & 0xFFFFF000; // Align by page
	if(to==0x0) return NULL; // Do not allow mapping 0x0
	return kalloc_page_from(kalloc_page_frame(), to);
}

void kfree_page(vaddr_t from)
{
	physaddr_t paddr=get_page(from) & 0xFFFFF000;
	kfree_page_frame(paddr);
	set_page(from, 0);
}
