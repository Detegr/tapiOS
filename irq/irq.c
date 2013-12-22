#include "irq.h"
#include <task/tss.h>
#include <util/scancodes.h>
#include <util/util.h>
#include <task/process.h>
#include <syscall/syscalls.h>
#include <stdint.h>

void timer_handler(void)
{
	if(!current_process) return;

	uint32_t esp, ebp, eip;
	__asm__ volatile("mov %0, esp;"
					 "mov %1, ebp;" : "=r"(esp), "=r"(ebp));

	eip=_get_eip();
	if(eip==0xADDEDBEE)
	{
		return; // Process switch occurred
	}

	// Switch process
	current_process->eip=eip;
	current_process->esp=esp;
	current_process->ebp=ebp;
	if(!current_process->next) current_process=process_list;
	else current_process=current_process->next;

	eip = current_process->eip;
	esp = current_process->esp;
	ebp = current_process->ebp;
	current_pdir=current_process->pdir;
	physaddr_t pageaddr=get_page((vaddr_t)current_pdir) & 0xFFFFF000;
	tss.esp0=((vaddr_t)current_process->esp0)+KERNEL_STACK_SIZE;

	// Handle EOI here before switching the process
	__asm__ volatile("cli;"
					 "call pic_get_irq;"
					 "cmp al, 0xFF;"
					 "je _panic;"
					 "mov bl, al;"
					 "mov al, 0x20;"
					 "out 0x20, al;"
					 "cmp bl, 0x8;"
					 "jge .send_slave;"
					 "jmp .finish;"
					 ".send_slave: out 0xA0, al;"
					 ".finish:");
	__asm__ volatile("mov ecx, %0;"
					 "mov esp, %1;"
					 "mov ebp, %2;"
					 "mov cr3, %3;"
					 "mov eax, 0xADDEDBEE;"
					 "sti;"
					 "jmp ecx;"
					 :: "r"(eip), "r"(esp), "r"(ebp), "r"(pageaddr) : "eax","ecx");
}

void irq1_handler(void)
{
	uint8_t status=inb(0x64);
	if(status & 0x1)
	{
		uint8_t scancode=inb(0x60);
		process* p=find_active_process();
		if(!p) return; // No active userspace process, nothing to do
		p->keybuf[p->keyp++]=scancode;
	}
	/*
	uint8_t i=inb(0x61);
	outb(0x61, i);
	*/
}
void page_fault(int errno)
{
	vaddr_t addr;
	__asm__ volatile("mov %0, cr2" : "=r"(addr));
	kprintf("Page fault in %s mode when %s%s.\nTried to access %x\n", errno & 0x1 ? "user" : "kernel", errno & 0x2 ? "writing" : "reading", errno & 0x4 ? ", protection violation" : "", addr);
	PANIC();
}
