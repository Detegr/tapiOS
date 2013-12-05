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


vptr_t* kalloc_page(vaddr_t to);
vptr_t* kalloc_page_from(physaddr_t from, vaddr_t to);
void kfree_page(vaddr_t from);
void setup_vmm(void);
page_directory* DEBUG_get_kernel_pdir(void);
page_directory* clone_page_directory_from(page_directory* src);

#endif
