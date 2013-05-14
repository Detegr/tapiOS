#include "util.h"
#include "video.h"

extern void _outb(uint8_t dest, uint16_t src);
extern uint8_t _inb(uint16_t port);
extern void _io_wait(void);
extern void _kb_int(void);
extern void _spurious_irq_check_master(void);
extern void _spurious_irq_check_slave(void);
extern void _noop_int(void);

void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags)
{
	// Setup GDT entry
	struct gdt_entry* ge=&gdt[n];
	ge->limit_low=limit & 0xFFFF;
	ge->limit_and_flags=((flags & 0x0F) << 4) | ((limit >> 28) & 0x0F);
	ge->base_low=base & 0xFFFF;
	ge->base_mid=(base >> 16) & 0xFF;
	ge->base_hi=(base >> 24) & 0xFF;
	ge->access=access;
}

void idtentry(int n, uint32_t offset, uint16_t selector, uint8_t type)
{
	struct idt_entry* ie=&idt[n];
	ie->offset_low = offset & 0xFFFF;
	ie->offset_hi = (offset >> 16) & 0xFFFF;
	ie->selector = selector;
	ie->zero = 0;
	ie->type_and_attr = type;
}

inline void outb(uint16_t port, uint8_t src)
{
	_outb(port, src);
}

uint8_t inb(uint16_t port)
{
	register uint8_t i=_inb(port);
	return i;
}

void panic(void)
{
	printk("Kernel panic, halting!\n");
}
