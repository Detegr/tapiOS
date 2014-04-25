#include "pmm.h"
#include <terminal/vga.h>

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
	print_startup_info("PMM", true);
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
		/*
		if(is_free_page(addr)) kprintf("Page still free\n");
		else kprintf("Page reserved OK\n");
		*/
		return addr;
	}
	else
	{
		kprintf("Page at %x not free!\n", addr);
		PANIC();
	}
	return 0;
}

physaddr_t kalloc_page_frame(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i)
	{
		if(bitmap[i] != 0xFFFFFFFF)
		{
			int msb=0;
			GET_AND_SET_MSB(msb, &bitmap[i]);
			return (i * 0x1000 * 32) + ((32-msb) * 0x1000);
		}
	}
	return (physaddr_t)0x0;
}

void kfree_page_frame(physaddr_t addr)
{
	addr=addr&0xFFFFF000; // Align
	if(addr < kernel_end_addr)
	{
		kprintf("Warning: Refusing to free physical address %x used by the kernel!\n", addr);
		return;
	}
	if(!is_free_page(addr))
	{
		set_page_reserved(addr, false);
		kprintf("Physical address %x freed.\n", addr);
	}
	else
	{
		kprintf("Double freeing page frame at %x\n", addr);
		PANIC();
	}
}
