#include "util.h"
#include "vga.h"
#include "irq.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "process.h"
#include "timer.h"
#include "tss.h"

#define KERNEL_VMA 0xC0000000

struct multiboot
{
   uint32_t flags;
   uint32_t mem_lower;
   uint32_t mem_upper;
   uint32_t boot_device;
   uint32_t cmdline;
   uint32_t mods_count;
   uint32_t mods_addr;
   uint32_t num;
   uint32_t size;
   uint32_t addr;
   uint32_t shndx;
   uint32_t mmap_length;
   uint32_t mmap_addr;
   uint32_t drives_length;
   uint32_t drives_addr;
   uint32_t config_table;
   uint32_t boot_loader_name;
   uint32_t apm_table;
   uint32_t vbe_control_info;
   uint32_t vbe_mode_info;
   uint32_t vbe_mode;
   uint32_t vbe_interface_seg;
   uint32_t vbe_interface_off;
   uint32_t vbe_interface_len;
}  __attribute__((packed));

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
	tssentry(5, 0x0, KERNEL_DATA_SELECTOR);
	gdtptr.limit=sizeof(gdt)-1;
	gdtptr.base=(unsigned int)&gdt;
	_setgdt();
	__asm__ volatile(
		"mov ax, 0x2B;" // 5(gdt entry num)*8(bytes) | 0x03 (privilege level)
		"ltr ax;" // Load task state register
	);
	print_startup_info("GDT", "OK\n");
}

void setup_pic(void)
{
	remap_pic(0x20, 0x28);
	print_startup_info("PIC", "OK\n");
}

uint32_t kernel_end_addr=0;
extern uint32_t __kernel_end;

void kmain(struct multiboot* b, uint32_t magic)
{
	cls();
	setup_gdt();
	setup_pic();
	setup_idt();
	if(b->mods_count == 1)
	{
		uint32_t mods_start_addr=*(uint32_t*)(b->mods_addr);
		uint32_t mods_end_addr=*(uint32_t*)(b->mods_addr + 4);
		if(((uint32_t)&__kernel_end - KERNEL_VMA) < mods_end_addr) kernel_end_addr=(mods_end_addr & 0xFFFFF000) + 0x1000;
	}
	setup_bitmap();
	setup_vmm();
	set_timer_freq(100);
	setup_multitasking();

	b=(struct multiboot*)((uint8_t*)b+KERNEL_VMA);
	uint32_t mods_addr=*(uint32_t*)(b->mods_addr + KERNEL_VMA) + KERNEL_VMA;

	kprintf("\n%@Welcome to tapiOS!%@\nMod count: %d\n%@%s%@", 0x05, 0x07, b->mods_count, 0x03, (char*)mods_addr, 0x07);

	int pid=fork();
	kprintf("Forked, pid: %d\n", pid);
	kprintf("Switching to user-mode...\n");
	switch_to_usermode();

	while(1)
	{
	}
	PANIC();
}
