#include "irq.h"
#include "scancodes.h"
#include "util.h"
#include "process.h"
#include "syscalls.h"
#include <stdint.h>

extern void _timer_handler(void);
extern void _irq1_handler(void);
extern void _page_fault(void);
extern void _syscall(void);

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

void setup_idt(void)
{
	int i;
	for(i=0; i<255; ++i)
	{
		idtentry(i, (uint32_t)&_noop_int, KERNEL_CODE_SELECTOR, GATE_INT32);
	}

	idtentry(0x0E, (uint32_t)&_page_fault, KERNEL_CODE_SELECTOR, TRAP_INT32);
	idtentry(0x20, (uint32_t)&_timer_handler, KERNEL_CODE_SELECTOR, GATE_INT32);
	idtentry(0x21, (uint32_t)&_irq1_handler, KERNEL_CODE_SELECTOR, GATE_INT32);
	idtentry(0x27, (uint32_t)&_spurious_irq_check_master, KERNEL_CODE_SELECTOR, GATE_INT32);
	idtentry(0x2F, (uint32_t)&_spurious_irq_check_slave, KERNEL_CODE_SELECTOR, GATE_INT32);
	idtentry(0x80, (uint32_t)&_syscall, KERNEL_CODE_SELECTOR, GATE_INT32_USER_PRIVILEGE);

	idtptr.limit=sizeof(idt)-1;
	idtptr.base=(unsigned int)&idt;
	_setidt();
	print_startup_info("IDT", true);
}

#define PIC1 0x20
#define PIC1_DATA 0x21
#define PIC2 0xA0
#define PIC2_DATA 0xA1

#define ICW4_NEEDED 0x1
#define ICW1_INIT 0x10

#define ICW4_80X86_MODE 0x1

#define EOI 0x20 // End Of Interrupt

void remap_pic(uint8_t offset1, uint8_t offset2)
{
	outb(PIC1, ICW4_NEEDED|ICW1_INIT);
	outb(PIC2, ICW4_NEEDED|ICW1_INIT);
	outb(PIC1_DATA, offset1); // New offset (for first 8 IRQs)
	outb(PIC2_DATA, offset2); // New offset (for the rest 8 IRQs)
	outb(PIC1_DATA, 0x04); // New slave in IRQ2
	outb(PIC2_DATA, 0x02); // Slave's master in IRQ1
	outb(PIC1_DATA, ICW4_80X86_MODE);
	outb(PIC2_DATA, ICW4_80X86_MODE);

	outb(PIC1_DATA, 0xFC); // Use IRQ0 and IRQ1
	outb(PIC2_DATA, 0xFF);

	__asm__ __volatile__("sti\n");
}

uint8_t pic_get_irq(void)
{
	outb(PIC1, 0x0B);
	uint8_t data=inb(PIC1);
	uint8_t i;
	uint8_t bit;
	for(i=data, bit=0; i!=0; i>>=1, bit++)
	{
		if(i & 0x1 && bit!=2) return bit;
	}
	data=inb(PIC2);
	for(i=data; i!=0; i>>=1, bit++)
	{
		if(i & 0x1) return bit;
	}
	return 0xFF;
}

uint8_t pic1_get_isr(void)
{
	outb(PIC1, 0x0B); // Read ISR
	return inb(PIC1);
}

uint8_t pic2_get_isr(void)
{
	outb(PIC2, 0x0B); // Read ISR
	return inb(PIC2);
}

uint8_t is_spurious_irq_master(void)
{
	uint8_t isr=pic1_get_isr();
	if(isr & 0x80) return 0;
	else return 1;
}

uint8_t is_spurious_irq_slave(void)
{
	uint8_t isr=pic2_get_isr();
	if(isr & 0x80) return 0;
	else return 1;
}
