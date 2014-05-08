#include "process.h"
#include "tss.h"
#include <mem/kmalloc.h>
#include <util/util.h>
#include <terminal/vga.h>
#include "elf.h"
#include "processtree.h"

#define PUSH(x) --stack_top; *stack_top=(uint32_t)x;

static uint32_t nextpid=2;
extern page_directory* kernel_pdir;
extern void _return_to_userspace(void);
extern void _return_to_userspace_from_syscall(void);

void setcwd(struct process *p, const char *path)
{
	strncpy(p->cwd, path, PATH_MAX);
}

void setcwd_dirname(struct process *p, const char *executable)
{
	char *buf=kmalloc(PATH_MAX);
	memset(buf, 0, PATH_MAX);
	strncpy(buf, executable, PATH_MAX);
	char *ptr=strtok(buf, '/');
	int i=0;
	while(ptr)
	{
		char *nextptr=strtok(NULL, '/');
		if(!nextptr) break;
		p->cwd[i++]='/';
		int len=strlen(ptr)+1;
		strncpy(&p->cwd[i], ptr, len);
		i+=len;
		ptr=nextptr;
	}
	kfree(buf);
}

static void copy_open_resources(struct process *from, struct process *to)
{
	for(int i=0; i<FD_MAX; ++i)
	{
		if(from->fds[i])
		{
			to->fds[i]=from->fds[i];
			to->fds[i]->refcount++;
		}
		else to->fds[i]=0;
	}
}

vptr_t *setup_child_stack(vptr_t *stack_top_ptr)
{
	uint32_t *stack_top=(uint32_t*)stack_top_ptr;

	/* These are for _return_to_userspace function
	 * that does popping the segment registers and
	 * popa for general purpose registers. */

	for(int i=0; i<4; ++i)
	{// ds,es,fs,gs to user mode DS
		PUSH(0x23);
	}

	/* These are for returning from irq_handler for the first time.
	 * irq_handler calls popa so we need to push initial register values
	 * and then it does ret, where in the first time we want to jump to
	 * _return_to_userspace instead of the label 1. */
	PUSH((uint32_t)_return_to_userspace);
	for(int i=0; i<8; ++i)
	{// Initial register values
		PUSH(0);
	}
	return (vptr_t*)stack_top;
}

int fork(void)
{
	__asm__ volatile("cli;");

	struct process *parent=(struct process*)current_process;
	page_directory* pdir=clone_page_directory_from(current_process->pdir);

	change_pdir(pdir);

	struct process *child=kmalloc(sizeof(struct process));
	memset(child, 0, sizeof(struct process));
	child->state=created;
	child->active=false;
	child->pid=nextpid++;
	child->pdir=pdir;
	child->esp0=kmalloc(KERNEL_STACK_SIZE);
	child->kesp=child->esp0 + KERNEL_STACK_SIZE;

	/* Copy register values from the parent,
	 * return 0 in eax for the child. */
	struct regs *pregs = (struct regs*)(parent->esp0 + KERNEL_STACK_SIZE - sizeof(struct regs));
	struct regs *cregs = (struct regs*)(child->kesp - sizeof(struct regs));
	cregs->edi    = pregs->edi;
	cregs->esi    = pregs->esi;
	cregs->ebp    = pregs->ebp;
	cregs->ebx    = pregs->ebx;
	cregs->edx    = pregs->edx;
	cregs->ecx    = pregs->ecx;
	cregs->eax    = 0;
	cregs->eip    = pregs->eip;
	cregs->cs     = pregs->cs;
	cregs->eflags = pregs->eflags;
	cregs->esp    = pregs->esp;
	cregs->ss     = pregs->ss;

	child->kesp=setup_child_stack((vptr_t*)cregs);
	child->brk=parent->brk;
	child->user_stack=parent->user_stack;
	strncpy(child->cwd, parent->cwd, PATH_MAX);

	memset(child->stdoutbuf, 0, 256);
	memset(child->keybuf, 0, 256);

	copy_open_resources(parent, child);

	struct process* p=(struct process*)process_list;
	while(p->next) p=p->next;
	p->next=child;

	insert_process_to_process_tree(child, parent);
	child->state=created;

	change_pdir(current_process->pdir);

	__asm__ volatile("sti;");
	return child->pid;
}

int getpid(void)
{
	return current_process->pid;
}

struct process *find_active_process(void)
{
	struct process *p=(struct process*)process_list;
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
	for(int i=0; i<FD_MAX; ++i)
	{
		if(current_process->fds[i] == NULL)
		{
			current_process->fds[i]=f;
			return i;
		}
	}
	return -1;
}

vptr_t *setup_usermode_stack(vaddr_t entry_point, int argc, char **const argv, char **const envp, vptr_t *stack_top_ptr)
{
	uint32_t *stack_top=(uint32_t*)stack_top_ptr;

	/* This part of the stack is the stack to iret to user mode in x86 processors.
	 * It contains (bottom-up):
	 * 	Ring3 EIP
	 * 	Ring3 CS
	 * 	EFLAGS
	 * 	Ring3 ESP
	 * 	Ring3 DS
	 *  Argc
	 *  Argv
	 * 	*/

	if(argv) PUSH(argv);
	if(argc) PUSH(argc);
	PUSH(envp);

	// 0x3 is the user privilege level
	PUSH(USER_DATA_SEGMENT | 0x3);
	PUSH((uint32_t)(stack_top+1));
	uint32_t eflags; __asm__ volatile("pushfd; pop %0;" : "=r"(eflags));
	PUSH(eflags|0x200); // Enable interrupts in EFLAGS
	PUSH(USER_CODE_SEGMENT | 0x3);
	PUSH(entry_point);

	/* This part of the stack is for _return_to_userspace
	 * function that pops segment registers and does popa
	 * for general purpose registers. */
	for(int i=0; i<8; ++i)
	{// Initial register values
		PUSH(0);
	}
	for(int i=0; i<4; ++i)
	{// ds,es,fs,gs to user mode DS
		PUSH(USER_DATA_SEGMENT | 0x3);
	}

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

	int bss_start=0;
	int bss_end=0;
	for(int i=0; i<header.program_entries; ++i)
	{
		int page_offset_from_elf=programs[i].p_offset / 0x1000;
		for(uint32_t j=0; j<=programs[i].p_filesz/0x1000; ++j)
		{
			kalloc_page_from(vaddr_to_physaddr((vaddr_t)elf + (page_offset_from_elf * 0x1000) + (j*0x1000)), programs[i].p_vaddr + (j*0x1000), false, true);
		}
		bss_start+=programs[i].p_vaddr + programs[i].p_filesz;
		bss_end+=programs[i].p_vaddr + programs[i].p_memsz;
	}

	/* It seems like newlib wants the initial brk to be page aligned
	 * so we need to align the initial brk address to beginning of a new page after bss.
	 * It is then of course also required to allocate a new page because sbrk syscall
	 * expects to be operating in an already allocated page initially.
	 */

	/* These pages are for bss */
	int bss_pages = ((bss_end - bss_start) / 0x1000) + 1;
	for(int i=1; i<=bss_pages; ++i)
	{
		current_process->brk=(bss_start + i*0x1000) & 0xFFFFF000;
		kalloc_page(current_process->brk, false, true);
	}

	/* Assign process brk to page after bss */
	if(current_process->brk != ((bss_end + 0x1000) & 0xFFFFF000))
	{
		current_process->brk=(bss_end + 0x1000) & 0xFFFFF000;
		kalloc_page(current_process->brk, false, true);
	}

	current_process->program=elf;
	return header.entry;
}
