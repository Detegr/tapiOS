#include "process.h"
#include "tss.h"
#include <mem/heap.h>
#include <util/util.h>
#include <terminal/vga.h>
#include "elf.h"

static uint32_t nextpid=2;
extern page_directory* kernel_pdir;
extern void _return_to_userspace(void);

static void copy_open_resources(process *from, process *to)
{
	for(int i=0; i<FD_MAX; ++i)
	{
		to->fds[i]=from->fds[i];
	}
	if(from->files_open)
	{
		to->files_open=kmalloc(sizeof(struct open_files));
		struct open_files *to_of=to->files_open;
		struct open_files *of=from->files_open;
		while((of=of->next))
		{
			to_of->next=kmalloc(sizeof(struct open_files));
			memcpy(to_of->next, of, sizeof(struct open_files));
			to_of=to_of->next;
		}
	}
}

int fork(void)
{
	/*
	__asm__ volatile("cli;");

	process* parent=(process*)current_process;
	page_directory* pdir=clone_page_directory_from(current_process->pdir);
	process* child=kmalloc(sizeof(process));
	memset(child, 0, sizeof(process));
	child->active=false;
	child->ready=false;
	child->pid=nextpid++;
	child->pdir=pdir;
	child->esp0=kmalloc(KERNEL_STACK_SIZE);
	memcpy(child->esp0, parent->esp0, KERNEL_STACK_SIZE);
	memset(child->stdoutbuf, 0, 256);

	//copy_open_resources(parent, child);

	process* p=(process*)process_list;
	while(p->next) p=p->next;
	p->next=child;

	child->eip=(uint32_t)_return_to_userspace;
	child->ready=true;
	__asm__ volatile("mov %0, ebp;"
					 "mov %1, esp;"
					 "sti;" : "=r"(child->ebp), "=r"(child->esp));
	return child->pid;
	*/
	return -1;
}

int getpid(void)
{
	return current_process->pid;
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

vptr_t *setup_usermode_stack(vaddr_t entry_point, vptr_t *stack_top_ptr)
{
#define PUSH(x) --stack_top; *stack_top=x;
	uint32_t *stack_top=(uint32_t*)stack_top_ptr;
	PUSH(0x23); // User mode DS | 0x3
	PUSH((uint32_t)(stack_top+1)); // esp
	uint32_t eflags; __asm__ volatile("pushfd; pop %0;" : "=r"(eflags));
	PUSH(eflags|0x200); // EFLAGS
	PUSH(0x1B); // User mode CS | 0x3
	PUSH(entry_point); //
	for(int i=0; i<8; ++i)
	{// Initial register values
		PUSH(0);
	}
	for(int i=0; i<4; ++i)
	{// ds,es,fs,gs to user mode DS
		PUSH(0x23);
	}
	PUSH((uint32_t)_return_to_userspace);

	return (vptr_t*)stack_top;
}

vaddr_t init_elf_get_entry_point(uint8_t* elf)
{
	elf_header header=*(elf_header*)elf;

	char elf_magic[]={0x7F,'E','L','F'};
	if(memcmp(&header, elf_magic, sizeof(elf_magic)) != 0)
	{
		kprintf("Elf header magic doesn't match\n");
		return 0;
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

	return header.entry;
}
