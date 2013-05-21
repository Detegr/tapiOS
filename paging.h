#ifndef TAPIOS_PAGING_H_
#define TAPIOS_PAGING_H_

#include <stdint.h>

#define PAGECOUNT 1024

typedef struct page
{
	union
	{
		struct
		{
			union
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
				uint8_t flags8;
			};
			unsigned dummy : 4;
			unsigned addr : 20;
		};
		uint32_t value;
	};
}__attribute__((packed)) page_t;

typedef struct page_table
{
	page_t pages[PAGECOUNT];
} __attribute__((aligned(4096))) page_table_t;

typedef struct page_directory
{
	page_table_t page_tables[PAGECOUNT];
}__attribute__((aligned(4096))) page_directory_t;

void setup_paging(void);

#endif
