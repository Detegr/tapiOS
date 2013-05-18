#include "util.h"
#include "video.h"
#include "irq.h"
#include "pmm.h"
#include "vmm.h"

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

physptr_t* testalloc(void)
{
	physptr_t* pf=kalloc_page_frame();
	printk("Allocated: ");
	printix((uint32_t) pf);
	printk("\n");
	return pf;
}

void testfree(physptr_t* ptr)
{
	kfree_page_frame(ptr);
	printk("Freed ");
	printix((uint32_t) ptr);
	printk("\n");
}

void kmain(void)
{
	cls();
	setup_gdt();
	setup_idt();
	setup_pic();
	setup_bitmap();
	setup_vmm();
	printk("Welcome to tapiOS!\n");

	testfree(testalloc());
	kalloc_page(0);

	while(1)
	{
		//__asm__("int $0x21\n");
		_idle();
	}
	panic();
}
