#include "util.h"

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

#define PIC1 0x20
#define PIC1_DATA 0x21
#define PIC2 0xA0
#define PIC2_DATA 0xA1

#define ICW4_NEEDED 0x1
#define ICW1_INIT 0x10

#define ICW4_80X86_MODE 0x1

void remap_pic(void)
{
	_outb(PIC1, ICW4_NEEDED|ICW1_INIT);
	_io_wait();
	_outb(PIC2, ICW4_NEEDED|ICW1_INIT);
	_io_wait();
	_outb(PIC1_DATA, 0x20); // New offset (for first 8 IRQs)
	_io_wait();
	_outb(PIC2_DATA, 0x28); // New offset (for the rest 8 IRQs)
	_io_wait();
	_outb(PIC1_DATA, 0x4); // New slave in IRQ2
	_io_wait();
	_outb(PIC2_DATA, 0x2); // Slave's master in IRQ1
	_io_wait();
	_outb(PIC1_DATA, ICW4_80X86_MODE);
	_io_wait();
	_outb(PIC2_DATA, ICW4_80X86_MODE);
}
