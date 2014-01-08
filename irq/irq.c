#include "irq.h"
#include <task/tss.h>
#include <task/scheduler.h>
#include <util/scancodes.h>
#include <util/util.h>
#include <task/process.h>
#include <syscall/syscalls.h>
#include <stdint.h>

void timer_handler(void)
{
	if(!current_process) return;

	struct process *new_process=get_next_process();
	struct process *old_process=(struct process*)current_process;
	if(!new_process || old_process == new_process) return;

	current_process=new_process;
	current_pdir=new_process->pdir;
	physaddr_t pageaddr=get_page((vaddr_t)current_pdir) & 0xFFFFF000;
	tss.esp0=((vaddr_t)current_process->esp0)+KERNEL_STACK_SIZE;

	/* Handle EOI before switching the process */
	__asm__ volatile(
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
	/* Save 8 general purpose registers, save old kernel stack pointer
	 * and switch kernel stack pointers. Also change the page directory to
	 * the switced process' page directory. */
	__asm__ volatile(
		"lea edx, 1f;"
		"push edx;"
		"pusha;"
		"lea edx, %0;"
		"mov [edx], esp;"
		"mov cr3, %2;"
		"mov esp, %1;"
		"popa;"
		"ret;"
		"1:" :: "m"(old_process->kesp), "b"(new_process->kesp), "c"(pageaddr));
}

void irq1_handler(void)
{
	uint8_t status=inb(0x64);
	if(status & 0x1)
	{
		uint8_t scancode=inb(0x60);
		struct process* p=find_active_process();
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
