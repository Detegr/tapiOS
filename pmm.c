#include "pmm.h"
#include "util.h"
#include "video.h"

extern uint8_t __kernel_end;

void setup_bitmap(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i) bitmap[i]=0;
	kend_addr=(((uint32_t)&__kernel_end & 0xFFFFF000) + 0x1000 - 0xC0000000); // Align by page
	// Setup bitmap for kernel's space
	unsigned i = kend_addr >> 17;
	unsigned offset = (kend_addr % 0x20000) >> 12;

	for(unsigned j=0; j<i; ++j) bitmap[j]=0xFFFFFFFF;
	for(unsigned j=0; j<offset; ++j)
	{
		bitmap[i] |= (0x80000000 >> j);
	}
}

bool is_free_page(physaddr_t addr)
{
	unsigned i=addr>>17;
	unsigned offset=(addr % 0x20000) >> 12;

	return (bitmap[i] & (0x80000000 >> offset)) == 0;
}

physaddr_t kalloc_page_frame(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i)
	{
		if(bitmap[i] != 0xFFFFFFFF)
		{
			for(int bit=31, j=0; bit; bit--, j++)
			{
				if(((bitmap[i] >> bit) & 0x1) == 0)
				{
					bitmap[i] |= 1<<(31-j);
					return (i * 0x1000 * 32) + (j * 0x1000);
				}
			}
		}
	}
	return (physaddr_t)0x0;
}

void kfree_page_frame(physaddr_t addr)
{
	if(addr < kend_addr)
	{
		printk("Warning: Refusing to free physical address used by the kernel!\n");
		return;
	}
	unsigned i = addr>>17;
	unsigned offset=(addr % 0x20000) >> 12;
	if(bitmap[i] & (0x80000000 >> offset))
	{
		bitmap[i] &= ~(0x80000000 >> offset);
	}
	else
	{
		printk("Double freeing page frame!\n");
		panic();
	}
}
