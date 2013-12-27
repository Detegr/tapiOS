#include "gdt.h"
#include <task/tss.h>
#include <terminal/vga.h>
#include <util/util.h>

extern void _setgdt(void);

struct gdt_entry
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_and_flags;
	uint8_t base_hi;
}__attribute__((packed));

struct gdt_ptr
{
	unsigned short limit;
	unsigned int base;
}__attribute__((packed));

struct gdt_entry gdt[6];
struct gdt_ptr gdtptr;

static void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags)
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

void tssentry(uint32_t n, uint16_t esp0, uint16_t ss0)
{
	memset(&tss - 0xC0000000, 0, sizeof(tss));

	uint32_t base=(uint32_t)&tss;
	uint32_t limit=base+sizeof(tss);
	gdtentry(n, base, limit, 0x89, 0x4); // 0x89: Present|Executable|Accessed, 0x4: size
	tss.esp0=esp0;
	tss.ss0=ss0;
	tss.cs = KERNEL_CODE_SEGMENT | 0x3; // Privilege mode 3
	tss.ss=tss.ds=tss.es=tss.fs=tss.gs=KERNEL_DATA_SEGMENT | 0x03;
}

static inline void setgdt(struct gdt_ptr *gdtptr)
{
	__asm__ volatile(
		"lgdt [%0];"
		"jmp 0x08:1f;"
		"1: mov ax, 0x10;" // 0x10 is the new data selector
		"mov ds,ax;"
		"mov es,ax;"
		"mov fs,ax;"
		"mov gs,ax;"
		"mov ss,ax;"
		:: "r"(gdtptr));
}

void setup_gdt(void)
{
	gdtentry(0, 0, 0, 0, 0); // null descriptor
	// Flat memory setup
	gdtentry(1, 0, 0xFFFFFFFF, 0x9A, 0x0F); // 0x9A == read only (code segment), flags: 4kb blocks, 32 bit protected mode (0x0F)
	gdtentry(2, 0, 0xFFFFFFFF, 0x92, 0x0F); // 0x92 == readwrite (data segment), flags: 4kb blocks, 32 bit protected mode (0x0F)
	// User mode entries
	gdtentry(3, 0, 0xFFFFFFFF, 0xFA, 0x0F); // User mode code segment (0x9A but privilege level 3)
	gdtentry(4, 0, 0xFFFFFFFF, 0xF2, 0x0F); // User mode data segment (0x92 but privilege level 3)
	// TSS
	tssentry(5, 0x0, KERNEL_DATA_SEGMENT);
	gdtptr.limit=sizeof(gdt)-1;
	gdtptr.base=(unsigned int)&gdt;
	setgdt(&gdtptr);
	__asm__ volatile(
		"mov ax, 0x2B;" // 5(gdt entry num)*8(bytes) | 0x03 (privilege level)
		"ltr ax;" // Load task state register
	);
	print_startup_info("GDT", true);
}
