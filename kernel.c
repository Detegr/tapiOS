#include "util.h"
#include "video.h"
#include "irq.h"
#include "bitmap.h"

void setup_gdt(void)
{
	printk("Setting up GDT...");
	gdtentry(0, 0, 0, 0, 0); // null descriptor
	// Flat memory setup
	gdtentry(1, 0, 0xFFFFFFFF, 0x9A, 0x0F); // 0x9A == read only (code segment), flags: 4kb blocks, 32 bit protected mode (0x0F)
	gdtentry(2, 0, 0xFFFFFFFF, 0x92, 0x0F); // 0x92 == readwrite (data segment), flags: 4kb blocks, 32 bit protected mode (0x0F)
	gdtptr.limit=sizeof(gdt)-1;
	gdtptr.base=(unsigned int)&gdt;
	_setgdt();
	printk("OK!\n");
}

void setup_pic(void)
{
	printk("Remapping PIC...");
	remap_pic(0x20, 0x28);
	printk("OK!\n");
}

void kmain(void)
{
	cls();
	setup_gdt();
	setup_idt();
	setup_pic();
	setup_bitmap();
	printk("Welcome to tapiOS!\n");

	physptr_t* pf=kalloc_page_frame();
	printk("Allocated: ");
	printix((uint32_t) pf);
	printk("\nFreed\n");
	pf=kalloc_page_frame();
	printix((uint32_t) pf);
	printk("Allocated: ");
	printix((uint32_t) pf);
	printk("\n");

	pf=kalloc_page_frame();
	printix((uint32_t) pf);
	printk("Allocated: ");
	printix((uint32_t) pf);
	printk("\n");

	pf=kalloc_page_frame();
	printix((uint32_t) pf);
	printk("Allocated: ");
	printix((uint32_t) pf);
	printk("\n");

	kfree_page_frame();
	kfree_page_frame();
	kfree_page_frame();

	pf=kalloc_page_frame();
	printix((uint32_t) pf);
	printk("Allocated: ");
	printix((uint32_t) pf);
	printk("\n");

	while(1)
	{
		//__asm__("int $0x21\n");
		_idle();
	}
	panic();
}
