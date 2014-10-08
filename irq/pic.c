#include "pic.h"
#include "idt.h"
#include "irq.h"
#include <task/process.h>
#include <task/scheduler.h>
#include <util/util.h>
#include <terminal/vga.h>

extern void _timer_handler(void);

#define ICW4_NEEDED 0x1
#define ICW1_INIT 0x10

#define ICW4_80X86_MODE 0x1

#define EOI 0x20 // End Of Interrupt

uint8_t pic_get_irq(void)
{
	outb(PIC1, 0x0B);
	uint16_t data=inb(PIC1);
	uint16_t ret;
	if(data==0x4)
	{// Chained to slave PIC
		outb(PIC2, 0x0B);
		data=inb(PIC2);
		GET_LSB(ret, data);
		return (uint8_t)ret + 0x8;
	}
	GET_LSB(ret, data);
	return (uint8_t)ret;
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

	// Interrupt masks
	outb(PIC1_DATA, 0xFF-0x1); // 0x1 is timer
	outb(PIC2_DATA, 0xFF);

	__asm__ __volatile__("sti\n");
}

void timer_handler(void)
{
	if(!current_process) return;

	/* Handle EOI before switching the process */
	__asm__ volatile(
		"call pic_get_irq;"
		"cmp al, 0xFF;"
		"je _panic;"
		"mov bl, al;"
		"mov al, 0x20;"
		"out 0x20, al;"
		"cmp bl, 0x8;"
		"jge .send_slave;"
		"jmp .finish;"
		".send_slave: out 0xA0, al;"
		".finish:");

	switch_task();
}

inline void setup_pic(void)
{
	remap_pic(0x20, 0x28);
	// Timer handler is different from other ISRs so we will handle it more low level
	idtentry(0x20, (uint32_t)&_timer_handler, KERNEL_CODE_SEGMENT, GATE_INT32);
	print_startup_info("PIC", true);
}
