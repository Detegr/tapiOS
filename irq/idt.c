#include "idt.h"
#include "gdt.h"
#include "irq.h"
#include <terminal/vga.h>
#include <stdint.h>
#include <util/util.h>

extern void _irq11_handler(void);
extern void _page_fault(void);
extern void _noop_int(void);

/************************/

extern void _syscall(void);
extern void _spurious_irq_check_master(void);
extern void _spurious_irq_check_slave(void);

struct idt_entry
{
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_and_attr;
	uint16_t offset_hi;
}__attribute__((packed));

struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
}__attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtptr;

void idtentry(int n, uint32_t offset, uint16_t selector, uint8_t type)
{
	struct idt_entry* ie=&idt[n];
	ie->offset_low = offset & 0xFFFF;
	ie->offset_hi = (offset >> 16) & 0xFFFF;
	ie->selector = selector;
	ie->zero = 0;
	ie->type_and_attr = type;
}

static inline void setidt(struct idt_ptr *idtptr)
{
	__asm__ volatile("lidt [%0];" :: "r"(idtptr));
}

void setup_idt(void)
{
	int i;
	for(i=0; i<255; ++i)
	{
		idtentry(i, (uint32_t)&_noop_int, KERNEL_CODE_SEGMENT, GATE_INT32);
	}
	isrs=NULL;

	idtentry(0x0E, (uint32_t)&_page_fault, KERNEL_CODE_SEGMENT, TRAP_INT32);
	idtentry(0x27, (uint32_t)&_spurious_irq_check_master, KERNEL_CODE_SEGMENT, GATE_INT32);
	idtentry(0x2F, (uint32_t)&_spurious_irq_check_slave, KERNEL_CODE_SEGMENT, GATE_INT32);
	idtentry(0x80, (uint32_t)&_syscall, KERNEL_CODE_SEGMENT, GATE_INT32_USER_PRIVILEGE);

	idtptr.limit=sizeof(idt)-1;
	idtptr.base=(unsigned int)&idt;
	setidt(&idtptr);
	print_startup_info("IDT", true);
}
