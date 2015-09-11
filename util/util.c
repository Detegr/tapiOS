#include "util.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>
#include <stdarg.h>

extern void _outb(uint16_t dest, uint8_t src);
extern void _outw(uint16_t dest, uint16_t src);
extern void _outdw(uint16_t dest, uint32_t src);
extern uint8_t _inb(uint16_t port);
extern uint16_t _inw(uint16_t port);
extern uint32_t _indw(uint16_t port);
extern void _io_wait(void);
extern void _kb_int(void);
extern void _noop_int(void);
extern void _panic(void);

inline void outb(uint16_t port, uint8_t src)
{
	_outb(port, src);
}

inline void outw(uint16_t port, uint16_t src)
{
	_outw(port, src);
}

inline void outdw(uint16_t port, uint32_t src)
{
	_outdw(port, src);
}

uint8_t inb(uint16_t port)
{
	register uint8_t i=_inb(port);
	return i;
}

uint16_t inw(uint16_t port)
{
	register uint16_t i=_inw(port);
	return i;
}

uint32_t indw(uint16_t port)
{
	register uint32_t i=_indw(port);
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
	if(src == dst) return dst;
	if(src > dst) memcpy(dst, src, size);
	else
	{
		uint8_t* p1=(uint8_t*)dst;
		uint8_t* p2=(uint8_t*)src;
		for(uint32_t i=size; i>0; --i)
		{
			p1[i]=p2[i];
		}
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

char *strncpy(char *dst, const char *src, uint32_t n)
{
	char *p1=dst;
	const char *p2=src;
	for(uint32_t i=0; i<n; ++i)
	{
		p1[i]=p2[i];
		if(p2[i] == '\0') break;
	}
	return dst;
}

int strncmp(const char *lhs, const char *rhs, uint32_t n)
{
	for(const char *p=lhs, *pp=rhs; n>0; --n, ++p, ++pp)
	{
		if(*p < *pp) return -1;
		if(*p > *pp) return 1;
	}
	return 0;
}

int strlen(const char* str)
{
	int len=0;
	while(*str++) len++;
	return len;
}

int strnlen(const char* str, uint32_t n)
{
	int len=0;
	while(n>0 && *str++)
	{
		len++;
		n--;
	}
	return len;
}

char *strndup(const char *str, uint32_t len)
{
	int strlen=strnlen(str, len);
	char *ret=kmalloc(strlen+1);
	strncpy(ret, str, len);
	return ret;
}

char *strdup(const char *str)
{
	int len=strlen(str);
	char *ret=kmalloc(len+1);
	strncpy(ret, str, len);
	return ret;
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

char *strchr(const char *str, char c)
{
	for(const char *p=str; *p; ++p)
	{
		if(*p == c) return (char*)p;
	}
	return NULL;
}

char *strrchr(const char *str, char c)
{
	const char *pp=NULL;
	for(const char *p=str; *p; ++p)
	{// Not very efficient, but who cares? :)
		if(*p == c) pp=p;
	}
	return (char*)pp;
}

char *basename(char *path)
{
	char *ret=strrchr(path, '/');
	return ret ? ret+1 : path;
}

char *dirname(char *path)
{
	char *sep=strrchr(path, '/');
	if(!sep) return ".";
	if(*(sep+1) == 0) {*sep=0; sep=strrchr(path, '/');}
	if(!sep || sep == path) return "/";
	*sep=0;
	return path;
}

inline int max(int a, int b)
{
	return a > b ? a : b;
}

inline int min(int a, int b)
{
	return a < b ? a : b;
}

inline int abs(int a)
{
	return a < 0 ? -a : a;
}

inline int isdigit(const char c)
{
	return c >= 48 && c <= 57;
}

int katoi(const char *str)
{// Yuck this function
	int ret=0;
	int len=strlen(str)-1;
	int digits[len+1];
	for(int i=len, j=0; i>=0; --i, ++j)
	{
		if(isdigit(str[i]))
		{
			digits[i] = str[i] - 48;
		}
		else return -1;
	}
	for(int i=0; i<len+1; ++i)
	{
		int val=digits[i];
		for(int j=0; j<len-i; ++j)
		{// Yuck this pow
			val *= 10;
		}
		ret += val;
	}
	return ret;
}

int ksscanf(const char *str, const char *fmt, char **outstr, ...)
{
	int elems=0;
	va_list args;
	va_start(args, outstr);
	const char *sp=str;
	for(const char *p=fmt; *p; ++p)
	{
		if(*p == '%')
		{
			switch(*(++p))
			{
				case 'u':
				{
					int digits;
					unsigned int *ret=va_arg(args, unsigned int*);
					const char *spp=sp;
					for(digits=0; sp && isdigit(*sp); ++sp) digits++;
					if(digits==0) goto skip;

					char buf[digits+1];
					for(int i=0; i<digits; ++i) buf[i]=spp[i];
					buf[digits]=0;
					*ret = katoi(buf);
					++elems;
				}
			}
		}
		else
		{
skip:
			if(*p != *sp) {outstr=NULL; return 0;}
			else ++sp;
		}
	}
	va_end(args);
	if(outstr) *outstr=(char*)sp;

	return elems;
}
