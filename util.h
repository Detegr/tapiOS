#include <stdint.h>

#define CODE_SELECTOR 0x08
#define DATA_SELECTOR 0x10

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
	unsigned short size;
	unsigned int base;
}__attribute__((packed));

struct idt_ptr
{
	unsigned short size;
	unsigned int base;
}__attribute__((packed));

struct gdt_entry gdt[3];
struct idt_entry idt[255];

struct gdt_ptr gdtptr;
struct idt_ptr idtptr;

void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags);
void setgdt();
