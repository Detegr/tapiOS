#include "util.h"

volatile unsigned char* video = (unsigned char*)0xB8000;

static unsigned char row=0;
static unsigned char line=0;

void cls(void)
{
	int x,y;
	for(x=0; x<0xA0; ++x)
	{
		for(y=0; y<0x32; ++y)
		{
			*(video+(y*0xA0)+x)=0;
			*(video+(y*0xA0)+x+1)=0;
		}
	}
}

void printk(const char* str)
{
	const char* s;
	for(s=str; *s; s++)
	{
		char c=*s;
		if(c=='\n')
		{
			row=0;
			line++;
			continue;
		}
		*(video+(line*0xA0)+row)=c;
		*(video+(line*0xA0)+row+1)=0x07; // Gray on black
		row+=2;
		if(row>=0xA0)
		{
			row=0;
			line++;
		}
	}
}

void setup_interrupts(void)
{
	gdt_t gdt;
	idt_t idt;
	setgdt(&gdt, sizeof(gdt));
	setidt(&idt, sizeof(idt));
}

void kmain(unsigned long magic, unsigned long addr)
{
	setup_interrupts();
	cls();
	printk("Welcome to tapiOS!\n");
}
