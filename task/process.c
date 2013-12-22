#include "process.h"
#include "tss.h"
#include <mem/heap.h>
#include <util/util.h>
#include <terminal/vga.h>
#include "elf.h"

static uint32_t nextpid=1;
extern page_directory* kernel_pdir;

int fork(void)
{
	__asm__ volatile("cli;");

	process* parent=(process*)current_process;
	page_directory* pdir=clone_page_directory_from(current_process->pdir);
	process* new=kmalloc(sizeof(process));
	memset(new, 0, sizeof(process));
	new->active=false;
	new->pid=nextpid++;
	new->pdir=pdir;
	new->esp0=kmalloc(KERNEL_STACK_SIZE);
	memset(new->esp0, 0, KERNEL_STACK_SIZE);
	memset(new->stdoutbuf, 0, 256);

	process* p=(process*)process_list;
	while(p->next) p=p->next;
	p->next=new;

	uint32_t eip=_get_eip();
	if(current_process == parent)
	{
		uint32_t ebp, esp;
		__asm__ volatile("mov %0, ebp;"
						 "mov %1, esp" : "=r"(ebp), "=r"(esp));
		new->esp=esp;
		new->ebp=ebp;
		new->eip=eip;
		__asm__ volatile("sti;");
		return new->pid;
	}
	else
	{
		return 0;
	}
}

int getpid(void)
{
	return current_process->pid;
}

void switch_to_usermode(vaddr_t entrypoint)
{
	tss.esp0=((vaddr_t)current_process->esp0)+KERNEL_STACK_SIZE;
	current_process->active=true;
	__asm__ volatile(
		"cli;"
		"mov ax, 0x23;" // 0x20 (user data segment) | 0x03 (privilege level 3)
		"mov ds, ax;"
		"mov es, ax;"
		"mov fs, ax;"
		"mov gs, ax;"
		"mov eax, esp;"
		"push 0x23;" // Push user data segment
		"push eax;"
		"pushfd;" // Push eflags
		// Set interrupt flag enabled in eflags as we cannot use sti in user mode anymore
		"pop eax;"
		"or eax, 0x200;" // Set IF (interrupt flag)
		"push eax;" // Push eflags back
		"push 0x1B;" // 0x18 (user code segment) | 0x03 (privilege level 3)
		"mov eax, %0;"
		"push eax;"
		"iret;" // Return. Interrupts will be enabled as we changed eflags manually.
		:: "r"(entrypoint) : "eax");
}

process* find_active_process(void)
{
	process* p=(process*)process_list;
	while(!p->active)
	{
		if(!p) PANIC();
		if(!p->next) return NULL;
		p=p->next;
	}
	return p;
}

int newfd(struct file *f)
{
	// 0, 1, 2 always reserved for now
	for(int i=3; i<FD_MAX; ++i)
	{
		if(current_process->fds[i] == NULL)
		{
			current_process->fds[i]=f;
			return i;
		}
	}
	return -1;
}

void setup_usermode_process(uint8_t* elf)
{
	elf_header header=*(elf_header*)elf;

	char elf_magic[]={0x7F,'E','L','F'};
	if(memcmp(&header, elf_magic, sizeof(elf_magic)) != 0)
	{
		kprintf("Elf header magic doesn't match\n");
		return;
	}
	elf_program_entry* programs=(elf_program_entry*)(elf+header.program_table);
	elf_section_entry* sections=(elf_section_entry*)(elf+header.section_table);

	for(int i=0; i<header.program_entries; ++i)
	{
		int page_offset_from_elf=programs[i].p_offset / 0x1000;
		for(uint32_t j=0; j<=programs[i].p_filesz/0x1000; ++j)
		{
			kalloc_page_from(vaddr_to_physaddr((vaddr_t)elf + (page_offset_from_elf * 0x1000) + (j*0x1000)), programs[i].p_vaddr + (j*0x1000), false, true);
		}
	}

	/* It seems like newlib wants the initial brk to be page aligned
	 * so we need to align the initial brk address to beginning of a new page after bss.
	 * It is then of course also required to allocate a new page because sbrk syscall
	 * expects to be operating in an already allocated page initially.
	 */
	current_process->brk=((programs[0].p_vaddr + programs[0].p_filesz) + 0x1000) & 0xFFFFF000;
	kalloc_page(current_process->brk, false, true);

	switch_to_usermode(header.entry);
}
