#ifndef TAPIOS_PAGING_H_
#define TAPIOS_PAGING_H_

typedef struct page
{
	struct flags
	{
		unsigned Present : 1;
		unsigned ReadWrite : 1;
		unsigned User : 1;
		unsigned WriteThrough : 1;
		unsigned CacheDisabled : 1;
		unsigned Accessed : 1;
		unsigned Zero : 1;
		unsigned Ignored : 1;
	}__attribute__((packed)) flags;
	unsigned dummy : 4;
	unsigned addr : 20;
}__attribute__((packed)) page_t;

typedef struct page_table
{
	page_t pages[1024];
} __attribute__((aligned(4096))) page_table_t;

typedef struct page_directory
{
	page_table_t page_tables[1024];
}__attribute__((aligned(4096))) page_directory_t;

#endif
