#include "util.h"
#include <terminal/vga.h>

extern void _outb(uint16_t dest, uint8_t src);
extern void _outw(uint16_t dest, uint16_t src);
extern uint8_t _inb(uint16_t port);
extern void _io_wait(void);
extern void _kb_int(void);
extern void _noop_int(void);
extern void _panic(void);

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

int memcmp(const void* src1, const void* src2, uint32_t n)
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

int strlen(const char* str)
{
	int len=0;
	while(*str++) len++;
	return len;
}

char *strtok(char *str, const char delim)
{
	static char *lastp=NULL;
	if(!lastp && !str) return NULL;

	char* p=str?str:lastp;
	while(*p == delim) ++p;
	if(!*p) return NULL;
	char *oldp=p;
	for(; *p; ++p)
	{
		if(*p == delim)
		{
			*p=0;
			lastp=p+1;
			return oldp;
		}
	}
	lastp=NULL;
	return oldp;
}
