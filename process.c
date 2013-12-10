#include "process.h"
#include "heap.h"
#include "util.h"
#include "vga.h"
#include "vmm.h"
#include "util.h"

static uint32_t nextpid=1;
extern page_directory* kernel_pdir;
extern uint32_t stack;
static uint32_t* stack_end_ptr=&stack;

static void copy_kernel_stack(vaddr_t to, uint32_t size)
{
	for(uint32_t i=0x1000; i<=size; i+=0x1000)
	{
		kalloc_page(to-i, false, true);
	}
	vptr_t* toptr=(vptr_t*)to;

	uint32_t esp, ebp;
	__asm__ volatile("mov %0, esp;"
					 "mov %1, ebp;" : "=r"(esp),"=r"(ebp));

	vaddr_t stackend=(uint32_t)stack_end_ptr;
	vaddr_t stackstart=stackend+size;

	uint32_t offset=to-stackstart;
	uint32_t to_esp=esp+offset;
	uint32_t to_ebp=ebp+offset;

	memcpy(toptr-size, stack_end_ptr, size);

	vaddr_t toaddr=(vaddr_t)toptr;
	uint32_t* p=(uint32_t*)(toptr-size);
	for(uint32_t i=0; i<4096; i++) // 16k stack
	{
		if(p[i] >= stackend && p[i] <= stackstart)
		{
			p[i]+=offset;
		}
	}

	__asm__ volatile("mov esp, %0;"
					 "mov ebp, %1;" :: "r"(to_esp), "r"(to_ebp));
}

void setup_multitasking(void)
{
	__asm__ volatile("cli;");
	copy_kernel_stack(0xE0000000, 0x4000);

	process_list=kmalloc(sizeof(process));
	current_process=process_list;
	memset((void*)current_process, 0, sizeof(process));
	current_process->pid=nextpid++;
	current_process->pdir=current_pdir;
	current_process->esp0=kmalloc(KERNEL_STACK_SIZE);
	memset(current_process->esp0, 0, KERNEL_STACK_SIZE);
	__asm__ volatile("sti;");

	print_startup_info("Multitasking", "OK\n");
}

int fork(void)
{
	__asm__ volatile("cli;");

	process* parent=(process*)current_process;
	page_directory* pdir=clone_page_directory_from(current_process->pdir);
	process* new=kmalloc(sizeof(process));
	memset(new, 0, sizeof(process));
	new->pid=nextpid++;
	new->pdir=pdir;
	new->esp0=kmalloc(KERNEL_STACK_SIZE);
	memset(new->esp0, 0, KERNEL_STACK_SIZE);

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
		"pushf;" // Push eflags
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

