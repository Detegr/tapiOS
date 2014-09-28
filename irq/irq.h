#ifndef _TAPIOS_IRQ_H_
#define _TAPIOS_IRQ_H_

#include <util/util.h>
#include <terminal/vga.h>

struct registers
{
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
};

typedef void (*isr_function)(void*, struct registers*);
struct isr
{
	int irq;
	isr_function func;
	void *data;
	struct isr *next;
};

volatile struct isr *isrs;
void register_isr(int irq, isr_function func, void *data);

void generic_exception(int error);
void generic_isr(void*);

void setup_idt(void);
void remap_pic(uint8_t offset1, uint8_t offset2);

#endif
