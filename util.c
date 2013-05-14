#include "util.h"

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

#define PIC1 0x20
#define PIC1_DATA 0x21
#define PIC2 0xA0
#define PIC2_DATA 0xA1

#define ICW4_NEEDED 0x1
#define ICW1_INIT 0x10

#define ICW4_80X86_MODE 0x1

#define EOI 0x20 // End Of Interrupt

void remap_pic(void)
{
	outb(PIC1, ICW4_NEEDED|ICW1_INIT);
	outb(PIC2, ICW4_NEEDED|ICW1_INIT);
	outb(PIC1_DATA, 0x20); // New offset (for first 8 IRQs)
	outb(PIC2_DATA, 0x28); // New offset (for the rest 8 IRQs)
	outb(PIC1_DATA, 0x04); // New slave in IRQ2
	outb(PIC2_DATA, 0x02); // Slave's master in IRQ1
	outb(PIC1_DATA, ICW4_80X86_MODE);
	outb(PIC2_DATA, ICW4_80X86_MODE);

	outb(PIC1_DATA, 0xFD);
	outb(PIC2_DATA, 0xFF);

	__asm__ __volatile__("sti\n");
}

uint8_t inb(uint16_t port)
{
	register uint8_t i=_inb(port);
	return i;
}

uint8_t pic_get_irq(void)
{
	outb(PIC1, 0x0B);
	uint8_t data=inb(PIC1);
	uint8_t i;
	uint8_t bit;
	for(i=data, bit=0; i!=0; i>>=1, bit++)
	{
		if(i & 0x1) return bit;
	}
	data=inb(PIC2);
	for(i=data; i!=0; i>>=1, bit++)
	{
		if(i & 0x1) return bit;
	}
	return 0xFF;
}

static uint8_t pic1_get_isr(void)
{
	outb(PIC1, 0x0B); // Read ISR
	return inb(PIC1);
}

static uint8_t pic2_get_isr(void)
{
	outb(PIC2, 0x0B); // Read ISR
	return inb(PIC2);
}

uint8_t is_spurious_irq_master(void)
{
	uint8_t isr=pic1_get_isr();
	if(isr & 0x80) return 0;
	else return 1;
}

uint8_t is_spurious_irq_slave(void)
{
	uint8_t isr=pic2_get_isr();
	if(isr & 0x80) return 0;
	else return 1;
}
