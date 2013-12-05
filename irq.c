#include "irq.h"
#include "scancodes.h"
#include "util.h"
#include "process.h"
#include <stdint.h>

extern void _timer_handler(void);
extern void _irq1_handler(void);
extern void _page_fault(void);

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

	__asm__ volatile("cli;"
					 "mov ecx, %0;"
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
	static int j=0;
	uint8_t status=inb(0x64);
	if(status & 0x1)
	{
		uint8_t data=inb(0x60);
		char c=char_for_scancode(data);
		if(c && c!= CHAR_UP && c!= CHAR_BACKSPACE)
		{
			kprintf("%c", c);
		}
		else if(c == CHAR_BACKSPACE)
		{
			delete_last_char();
		}
		else if(c != CHAR_UP)
		{
			//kprintf("\nUnhandled char: %x\n", data);
		}
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
	kprintf("Page fault when trying to access %x\n", addr);
	PANIC();
}

void setup_idt(void)
{
	int i;
	for(i=0; i<255; ++i)
	{
		idtentry(i, (uint32_t)&_noop_int, CODE_SELECTOR, GATE_INT32);
	}

	idtentry(0x0E, (uint32_t)&_page_fault, CODE_SELECTOR, TRAP_INT32);
	idtentry(0x20, (uint32_t)&_timer_handler, CODE_SELECTOR, GATE_INT32);
	idtentry(0x21, (uint32_t)&_irq1_handler, CODE_SELECTOR, GATE_INT32);
	idtentry(0x27, (uint32_t)&_spurious_irq_check_master, CODE_SELECTOR, GATE_INT32);
	idtentry(0x2F, (uint32_t)&_spurious_irq_check_slave, CODE_SELECTOR, GATE_INT32);

	idtptr.limit=sizeof(idt)-1;
	idtptr.base=(unsigned int)&idt;
	_setidt();
	print_startup_info("IDT", "OK\n");
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
