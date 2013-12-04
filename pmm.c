#include "pmm.h"
#include "util.h"
#include "video.h"

extern uint32_t kernel_end_addr;
extern uint32_t __kernel_end;

void set_page_reserved(physaddr_t addr, bool set);

void setup_bitmap(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i) bitmap[i]=0;
	if(!kernel_end_addr)
	{
		kernel_end_addr=(((uint32_t)&__kernel_end & 0xFFFFF000) + 0x1000 - 0xC0000000); // Align by page
	}

	// Setup bitmap for kernel's space
	for(uint32_t i=0; i<kernel_end_addr; i+=0x1000)
	{
		set_page_reserved(i, true);
	}
	print_startup_info("PMM", "OK\n");
}

bool is_free_page(physaddr_t addr)
{
	unsigned i=addr>>17;
	unsigned offset=(addr % 0x20000) >> 12;

	return (bitmap[i] & (0x80000000 >> offset)) == 0;
}

void set_page_reserved(physaddr_t addr, bool set)
{
	unsigned i=addr>>17;
	unsigned offset=(addr % 0x20000) >> 12;
	if(set) bitmap[i] |= (0x80000000 >> offset);
	else bitmap[i] &= ~(0x80000000 >> offset);
}

physaddr_t kalloc_page_frame_at(physaddr_t addr)
{
	addr=addr&0xFFFFF000; // Align
	if(is_free_page(addr))
	{
		set_page_reserved(addr, true);
		if(is_free_page(addr)) printk("Page still free :G\n");
		else printk("Page reserved OK\n");
		return addr;
	}
	else
	{
		printk("Page at ");
		printix(addr);
		printk(" not free!\n");
	}
	return 0;
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
	addr=addr&0xFFFFF000; // Align
	if(addr < kernel_end_addr)
	{
		printk("Warning: Refusing to free physical address ");
		printix(addr);
		printk(" used by the kernel!\n");
		return;
	}
	if(!is_free_page(addr))
	{
		set_page_reserved(addr, false);
		printk("Physical address ");
		printix(addr);
		printk(" freed.");
	}
	else
	{
		printk("Double freeing page frame at\n");
		printix(addr);
		printk("\n");
		panic();
	}
}
