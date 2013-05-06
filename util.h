#include <stdint.h>

struct gdt_entry
{
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char base_mid;
	unsigned char access;
	unsigned char limit_and_flags;
	unsigned char base_hi;
}__attribute__((packed));

struct gdt_ptr
{
	unsigned short size;
	unsigned int base;
}__attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gdtptr;

void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags);
void setgdt();
