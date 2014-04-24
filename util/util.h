#ifndef _TAPIOS_UTIL_H_
#define _TAPIOS_UTIL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

#define PANIC() panic(__FILE__,__LINE__)

#define KERNEL_CODE_SEGMENT 0x08
#define KERNEL_DATA_SEGMENT 0x10
#define USER_CODE_SEGMENT 0x18
#define USER_DATA_SEGMENT 0x20

#define ENOMEM 12
#define EINVAL 22

typedef int32_t pid_t;

uint8_t inb(uint16_t port);

void outb(uint16_t port, uint16_t src);
void outw(uint16_t port, uint16_t src);
uint8_t inb(uint16_t port);

void panic(const char* file, uint32_t line);

void* memcpy(void* dst, const void* src, uint32_t size);
void* memmove(void* dst, const void* src, uint32_t size);
void* memset(void* dst, uint8_t c, uint32_t n);
int memcmp(const void* src1, const void* src2, uint32_t n);
char *strncpy(char *dst, const char *src, uint32_t n);
int strlen(const char* str);
int strnlen(const char* str, uint32_t n);
char *strtok(char *str, const char delim);
void dirname(char *dst, const char *path);
extern uint32_t _get_eip(void);

#endif
