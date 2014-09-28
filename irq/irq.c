#include "irq.h"
#include "idt.h"
#include "pic.h"
#include <task/tss.h>
#include <task/scheduler.h>
#include <util/scancodes.h>
#include <util/util.h>
#include <task/process.h>
#include <syscall/syscalls.h>
#include <stdint.h>
#include <drivers/keyboard.h>
#include <mem/kmalloc.h>

extern void _generic_isr(void*);

void register_isr(int irq, isr_function func, void *data)
{
	volatile struct isr *isr=isrs;
	if(isr)
	{
		while(isr->next) isr=isr->next;
		isr->next=kmalloc(sizeof(struct isr));
		isr=isr->next;
	}
	else
	{
		isr=kmalloc(sizeof(struct isr));
		isrs=isr;
	}
	isr->irq=irq;
	isr->func=func;
	isr->data=data;
	isr->next=NULL;

	idtentry(0x20 + irq, (uint32_t)&_generic_isr, KERNEL_CODE_SEGMENT, GATE_INT32);
}

void generic_isr(void *data)
{
	volatile struct isr *isr=isrs;
	int irq=pic_get_irq();
	while(isr && isr->irq != irq)
	{
		isr=(volatile struct isr*)isr->next;
	}
	if(!isr)
	{
		kprintf("No ISR found for IRQ %d\n", irq);
		PANIC();
	}
	isr->func(isr->data, (struct registers*)&data);
}

void page_fault(int errno)
{
	vaddr_t addr;
	__asm__ volatile("mov %0, cr2" : "=r"(addr));
	kprintf("Page fault in %s mode when %s%s.\nTried to access %x\n", errno & 0x1 ? "user" : "kernel", errno & 0x2 ? "writing" : "reading", errno & 0x4 ? ", protection violation" : ", page not present", addr);
	PANIC();
}
