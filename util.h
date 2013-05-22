#ifndef _TAPIOS_UTIL_H
#define _TAPIOS_UTIL_H

#include <stdint.h>
#include <stdbool.h>

#define NULL (void*)0x0

// GDT selectors
#define CODE_SELECTOR 0x08
#define DATA_SELECTOR 0x10

// Interrupt types
#define GATE_INT32 0x8E
#define TRAP_INT32 0x8F

typedef unsigned long physaddr_t;
typedef unsigned char physptr_t;

extern void _spurious_irq_check_master(void);
extern void _spurious_irq_check_slave(void);
extern void _kb_int(void);
extern void _noop_int(void);
extern void _idle(void);

struct gdt_entry
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_and_flags;
	uint8_t base_hi;
}__attribute__((packed));

struct idt_entry
{
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_and_attr;
	uint16_t offset_hi;
}__attribute__((packed));

struct gdt_ptr
{
	unsigned short limit;
	unsigned int base;
}__attribute__((packed));

struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
}__attribute__((packed));

struct gdt_entry gdt[3];
struct idt_entry idt[256];

struct gdt_ptr gdtptr;
struct idt_ptr idtptr;

void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags);
void idtentry(int n, uint32_t offset, uint16_t selector, uint8_t type);

uint8_t inb(uint16_t port);

void _setgdt(void);
void _setidt(void);

uint8_t is_spurious_irq_master(void);
uint8_t is_spurious_irq_slave(void);
void outb(uint16_t port, uint8_t src);
uint8_t inb(uint16_t port);

void panic(void);

#endif
