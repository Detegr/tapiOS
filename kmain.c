#include <util/util.h>
#include <terminal/vga.h>
#include <irq/gdt.h>
#include <irq/idt.h>
#include <irq/irq.h>
#include <irq/timer.h>
#include <irq/pic.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/heap.h>
#include <task/process.h>
#include <task/tss.h>
#include <syscall/syscalls.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <task/multitasking.h>

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

uint32_t kernel_end_addr=0;
extern uint32_t __kernel_end;

void kmain(struct multiboot* b, uint32_t magic)
{
	hide_cursor();
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
	setup_tasking();
	set_timer_freq(100);

	b=(struct multiboot*)((uint8_t*)b+KERNEL_VMA);
	uint32_t mods_addr=*(uint32_t*)(b->mods_addr + KERNEL_VMA) + KERNEL_VMA;
	root_fs=ext2_fs_init((uint8_t*)mods_addr);

	kprintf("\n%@Welcome to tapiOS!%@\n\n", 0x05, 0x07, b->mods_count, 0x03);

	struct inode *node=vfs_search((struct inode*)root_fs, "/bin/init");
	if(node)
	{
		struct file init;

		vfs_open(node, &init);
		uint8_t *init_mem=kmalloc(node->size);
		int read=vfs_read(&init, init_mem, node->size);
		vaddr_t entrypoint=init_elf_get_entry_point(init_mem);
		setup_initial_process(entrypoint);
	}
	else kprintf("Init not found\n");
	__asm__ volatile("hltloop: hlt; jmp hltloop");

	PANIC();
}
