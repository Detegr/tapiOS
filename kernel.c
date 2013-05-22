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
	if(is_free_page(0x100000))
		printk("Free page 0x100000\n");
	else printk("Not free page 0x100000\n");
	if(is_free_page(0x0))
		printk("Free page 0x0\n");
	else printk("Not free page 0x0\n");
	if(is_free_page(0x3FFFFF))
		printk("Free page 0x3FFFFF\n");
	else printk("Not free page 0x3FFFFF\n");
	if(is_free_page(0x400000))
		printk("Free page 0x400000\n");
	else printk("Not free page 0x400000\n");
	if(is_free_page(0x500000))
		printk("Free page 0x500000\n");
	else printk("Not free page 0x500000\n");
	while(1)
	{
		//__asm__("int $0x21\n");
		_idle();
	}
	panic();
}
