#ifndef _TAPIOS_UTIL_H
#define _TAPIOS_UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include "tss.h"

#define NULL (void*)0x0
#define PANIC() panic(__FILE__,__LINE__)

// GDT selectors
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

// Interrupt types
#define GATE_INT32 0x8E
#define TRAP_INT32 0x8F

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

struct gdt_entry gdt[6];
struct idt_entry idt[256];
struct tss_entry tss;

struct gdt_ptr gdtptr;
struct idt_ptr idtptr;

void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags);
void idtentry(int n, uint32_t offset, uint16_t selector, uint8_t type);
void tssentry(uint32_t n, uint16_t esp0, uint16_t ss0);

uint8_t inb(uint16_t port);

void _setgdt(void);
void _setidt(void);

uint8_t is_spurious_irq_master(void);
uint8_t is_spurious_irq_slave(void);
void outb(uint16_t port, uint16_t src);
void outw(uint16_t port, uint16_t src);
uint8_t inb(uint16_t port);

void panic(const char* file, uint32_t line);

void* memcpy(void* dst, const void* src, uint32_t size);
void* memmove(void* dst, const void* src, uint32_t size);
void* memset(void* dst, uint8_t c, uint32_t n);
int memcmp(void* src1, void* src2, uint32_t n);
extern uint32_t _get_eip(void);

#endif
