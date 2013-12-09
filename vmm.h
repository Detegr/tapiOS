#ifndef _TAPIOS_VMM_H_
#define _TAPIOS_VMM_H_

typedef unsigned char vptr_t;
typedef unsigned long vaddr_t;

typedef struct page_directory_entry
{
	union
	{
		struct
		{
			unsigned flags : 12;
			unsigned addr : 20;
		} __attribute__((packed));
		uint32_t as_uint32;
	};
} __attribute__((packed)) page_directory_entry ;

typedef uint32_t page_table_entry;
typedef struct page_table
{
	page_table_entry entries[1024];
} page_table;

typedef struct page_directory
{
	page_directory_entry entries[1024];
} page_directory;

page_directory* current_pdir;

vptr_t* kalloc_page(vaddr_t to, bool kernel, bool readwrite);
vptr_t* kalloc_page_from(physaddr_t from, vaddr_t to, bool kernel, bool readwrite);
void kfree_page(vaddr_t from);
void setup_vmm(void);
page_directory* DEBUG_get_kernel_pdir(void);
page_directory* clone_page_directory_from(page_directory* src);

void set_page_table_entry(uint16_t pdi, uint32_t pti, uint32_t flags);
void set_page(vaddr_t addr, uint32_t flags);
uint32_t get_page(vaddr_t addr);
void change_pdir(page_directory* pdir);

#endif
