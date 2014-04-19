#include "kmalloc.h"
#include "vmm.h"
#include <terminal/vga.h>

extern uint32_t kernel_end_addr;
static uint32_t kheap_end=0;
static uint32_t blockinfo_start=0;

static struct metadata *metadata;

struct metadata
{
	uint32_t blocks_allocated;
	uint32_t max_blocks;
	struct blockinfo *blocks;
	struct metadata *next;
};

struct blockinfo
{
	bool reserved;
	uint32_t size; // pages
	vptr_t *ptr;
};

static struct blockinfo *insert_block(uint32_t size, vptr_t *ptr)
{
	if(size == 0) PANIC();

	if(metadata->blocks_allocated+1 > metadata->max_blocks)
	{
		kprintf("NYI: Max blocks(%d) reached, allocate new metadata block\n", metadata->max_blocks);
		PANIC();
	}
	struct blockinfo *bi = &(metadata->blocks[metadata->blocks_allocated++]);
	bi->reserved = false;
	bi->size = size;
	bi->ptr = ptr;
	return bi;
}

static vptr_t *find_free_blocks(uint32_t amount)
{
	if(amount == 0) PANIC();
	for(uint32_t i=0; i<metadata->blocks_allocated; ++i)
	{
		struct blockinfo *bi = &metadata->blocks[i];
		if(!bi->reserved && bi->size >= amount)
		{
			if(amount < bi->size)
			{
				//kprintf("Splitting block %d %x, size: %d\n", i, bi->ptr, bi->size-amount);
				vptr_t *newptr = bi->ptr + ((bi->size - amount) * 0x1000);
				struct blockinfo *ret = insert_block(amount, newptr);
				ret->reserved=true;
				bi->size -= amount;
				return ret->ptr;
			}
			bi->reserved=true;
			return bi->ptr;
		}
	}
	return NULL;
}

void *kmalloc(size_t size)
{
	if(size==0) return NULL;
	if(!kheap_end)
	{
		kheap_end=(0xC0000000|kernel_end_addr)+0x1000; // Kernel heap starts where kernel code ends
		metadata=(struct metadata*)kalloc_page(kheap_end, true, true);
		metadata->blocks_allocated=0;
		metadata->max_blocks = (0x1000 - sizeof(struct metadata)) / sizeof(struct blockinfo);
		metadata->blocks = (struct blockinfo*)&metadata[1];
		metadata->next=NULL;
		kheap_end += 0x1000;
	}

	int amount;
	if(size % 0x1000 == 0) amount=size/0x1000;
	else amount=(size/0x1000)+1;
	if(amount==0) amount=1;

	vptr_t *hole = find_free_blocks(amount);
	if(hole)
	{
		if((vaddr_t)hole & 0x00000FFF) PANIC();
		return hole;
	}

	vptr_t* ret=NULL;
	for(int i=0; i<amount; ++i, kheap_end+=0x1000)
	{
		if(!ret) ret=kalloc_page(kheap_end, false, true);
		else kalloc_page(kheap_end, false, true);
	}
	struct blockinfo *bi=insert_block(amount, ret);
	bi->reserved=true;
	if((vaddr_t)ret & 0x00000FFF) PANIC();
	return ret;
}

void kfree(void *ptr)
{
	for(uint32_t i=0; i<metadata->blocks_allocated; ++i)
	{
		struct blockinfo *bi = &metadata->blocks[i];
		if(bi->ptr == ptr)
		{
			if(!bi->reserved)
			{
				kprintf("DOUBLE FREE: %x\n", ptr);
				PANIC();
			}
			//kprintf("Freed %x, size was: %d\n", ptr, bi->size);
			bi->reserved=false;
			return;
		}
	}
}
