#include "util.h"
#include "vga.h"

extern void _outb(uint16_t dest, uint8_t src);
extern void _outw(uint16_t dest, uint16_t src);
extern uint8_t _inb(uint16_t port);
extern void _io_wait(void);
extern void _kb_int(void);
extern void _spurious_irq_check_master(void);
extern void _spurious_irq_check_slave(void);
extern void _noop_int(void);
extern void _panic(void);

void gdtentry(int n, unsigned int base, unsigned int limit, unsigned char access, unsigned char flags)
{
	// Setup GDT entry
	struct gdt_entry* ge=&gdt[n];
	ge->limit_low=limit & 0xFFFF;
	ge->limit_and_flags=((flags & 0x0F) << 4) | ((limit >> 28) & 0x0F);
	ge->base_low=base & 0xFFFF;
	ge->base_mid=(base >> 16) & 0xFF;
	ge->base_hi=(base >> 24) & 0xFF;
	ge->access=access;
}

void idtentry(int n, uint32_t offset, uint16_t selector, uint8_t type)
{
	struct idt_entry* ie=&idt[n];
	ie->offset_low = offset & 0xFFFF;
	ie->offset_hi = (offset >> 16) & 0xFFFF;
	ie->selector = selector;
	ie->zero = 0;
	ie->type_and_attr = type;
}

inline void outb(uint16_t port, uint16_t src)
{
	_outb(port, src);
}

inline void outw(uint16_t port, uint16_t src)
{
	_outw(port, src);
}

uint8_t inb(uint16_t port)
{
	register uint8_t i=_inb(port);
	return i;
}

void panic(const char* file, uint32_t line)
{
	kprintf("Kernel panic (%s:%d), halting!\n", file, line);
	_panic();
}

void* memcpy(void* dst, const void* src, uint32_t size)
{
	uint8_t* p1=(uint8_t*)dst;
	uint8_t* p2=(uint8_t*)src;
	for(uint32_t i=0; i<size; ++i)
	{
		p1[i]=p2[i];
	}
	return dst;
}

void* memmove(void* dst, const void* src, uint32_t size)
{
	uint8_t buf[size];
	uint8_t* p1=(uint8_t*)dst;
	uint8_t* p2=(uint8_t*)src;
	for(uint32_t i=0; i<size; ++i)
	{
		buf[i]=p2[i];
	}
	for(uint32_t i=0; i<size; ++i)
	{
		p1[i]=buf[i];
	}
	return dst;
}

void* memset(void* dst, uint8_t c, uint32_t n)
{
	uint8_t* dp=(uint8_t*)dst;
	for(uint32_t i=0; i<n; ++i)
	{
		dp[i]=c;
	}
	return dst;
}

int memcmp(void* src1, void* src2, uint32_t n)
{
	uint8_t* p1=(uint8_t*)src1;
	uint8_t* p2=(uint8_t*)src2;
	for(uint32_t i=0; i<n; ++i)
	{
		if(p1[i] < p2[i]) return -1;
		else if(p1[i] > p2[i]) return 1;
	}
	return 0;
}
