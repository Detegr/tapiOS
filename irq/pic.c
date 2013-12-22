#include "pic.h"
#include <util/util.h>
#include <terminal/vga.h>

#define PIC1 0x20
#define PIC1_DATA 0x21
#define PIC2 0xA0
#define PIC2_DATA 0xA1

#define ICW4_NEEDED 0x1
#define ICW1_INIT 0x10

#define ICW4_80X86_MODE 0x1

#define EOI 0x20 // End Of Interrupt

uint8_t pic_get_irq(void)
{
	outb(PIC1, 0x0B);
	uint8_t data=inb(PIC1);
	uint8_t i;
	uint8_t bit;
	for(i=data, bit=0; i!=0; i>>=1, bit++)
	{
		if(i & 0x1 && bit!=2) return bit;
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

void remap_pic(uint8_t offset1, uint8_t offset2)
{
	outb(PIC1, ICW4_NEEDED|ICW1_INIT);
	outb(PIC2, ICW4_NEEDED|ICW1_INIT);
	outb(PIC1_DATA, offset1); // New offset (for first 8 IRQs)
	outb(PIC2_DATA, offset2); // New offset (for the rest 8 IRQs)
	outb(PIC1_DATA, 0x04); // New slave in IRQ2
	outb(PIC2_DATA, 0x02); // Slave's master in IRQ1
	outb(PIC1_DATA, ICW4_80X86_MODE);
	outb(PIC2_DATA, ICW4_80X86_MODE);

	outb(PIC1_DATA, 0xFC); // Use IRQ0 and IRQ1
	outb(PIC2_DATA, 0xFF);

	__asm__ __volatile__("sti\n");
}

inline void setup_pic(void)
{
	remap_pic(0x20, 0x28);
	print_startup_info("PIC", true);
}
