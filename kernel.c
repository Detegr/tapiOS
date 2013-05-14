#include "util.h"
#include "video.h"

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

void setup_idt(void)
{
	printk("Setting up IDT...");
	int i;
	for(i=0; i<255; ++i)
	{
		if(i<=0x1F) idtentry(i, (uint32_t)&_noop_int, CODE_SELECTOR, GATE_INT32);
		else if(i==0x21) idtentry(i, (uint32_t)&_kb_int, CODE_SELECTOR, GATE_INT32);
		else if(i==0x27) idtentry(i, (uint32_t)&_spurious_irq_check_master, CODE_SELECTOR, GATE_INT32);
		else if(i==0x2F) idtentry(i, (uint32_t)&_spurious_irq_check_slave, CODE_SELECTOR, GATE_INT32);
		else idtentry(i, (uint32_t)0, 0, 0);
	}
	idtentry(0x21, (uint32_t)&_kb_int, CODE_SELECTOR, GATE_INT32);
	idtptr.limit=sizeof(idt)-1;
	idtptr.base=(unsigned int)&idt;
	_setidt();
	printk("OK!\n");
}

void setup_pic(void)
{
	printk("Remapping PIC...");
	remap_pic();
	printk("OK!\n");
}

void test(void)
{
	printk("Interrupt test!\n");
	inb(0x60);
	uint8_t i=inb(0x61);
	outb(0x61, i);
}

void panic(void)
{
	printk("Kernel panic, halting!\n");
}

void kmain(unsigned long magic, unsigned long addr)
{
	cls();
	setup_gdt();
	setup_idt();
	setup_pic();
	printk("Welcome to tapiOS!\n");
	while(1)
	{
		//__asm__("int $0x21\n");
		_idle();
	}
	panic();
}
