#ifndef _TAPIOS_UTIL_H_
#define _TAPIOS_UTIL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

#undef errno // Only want the definitions from errno.h

#define PANIC() panic(__FILE__,__LINE__)

#define KERNEL_CODE_SEGMENT 0x08
#define KERNEL_DATA_SEGMENT 0x10
#define USER_CODE_SEGMENT 0x18
#define USER_DATA_SEGMENT 0x20

#define DIV_ROUND_UP(n,d) (n/d + (n%d != 0))
#define GET_AND_SET_MSB(out, bitmap_uint32_addr) \
	__asm__ volatile("bsf %0, %1" : "=g"(out) : "g"(*bitmap_uint32_addr)); \
	if(out > 0) *bitmap_uint32_addr |= 1<<(out-1); \
	else *bitmap_uint32_addr |= 1<<31; \
	if(out==0) out=32;
#define GET_AND_SET_LSB(out, bitmap_uint32_addr) \
	__asm__ volatile("bsr %0, %1" : "=g"(out) : "g"(*bitmap_uint32_addr)); \
	*bitmap_uint32_addr |= (1 << (++out));

typedef int32_t pid_t;

uint8_t inb(uint16_t port);

void outb(uint16_t port, uint8_t src);
void outdw(uint16_t port, uint32_t src);
void outw(uint16_t port, uint16_t src);
uint8_t inb(uint16_t port);
uint32_t indw(uint16_t port);

void panic(const char* file, uint32_t line);

void* memcpy(void* dst, const void* src, uint32_t size);
void* memmove(void* dst, const void* src, uint32_t size);
void* memset(void* dst, uint8_t c, uint32_t n);
int memcmp(const void* src1, const void* src2, uint32_t n);
char *strncpy(char *dst, const char *src, uint32_t n);
int strncmp(const char *lhs, const char *rhs, uint32_t n);
int strlen(const char* str);
int strnlen(const char* str, uint32_t n);
char *strndup(const char *str, uint32_t len);
char *strdup(const char *str);
char *strtok(char *str, const char delim);
char *basename(char *path); /* GNU version */
char *dirname(char *path);
extern uint32_t _get_eip(void);
int max(int a, int b);

int ksscanf(const char *str, const char *fmt, char **outstr, ...);
int isdigit(const char c);

#endif
