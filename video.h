#ifndef _TAPIOS_VIDEO_H_
#define _TAPIOS_VIDEO_H_

#include <stdint.h>

volatile unsigned char* video;

void cls(void);
void printk(const char* str);
void printkc(const char* str, uint8_t color);
void printix(uint32_t x);
void print_startup_info(const char* section, const char* msg);
void kprintf(const char* fmt, ...);
void hide_cursor(void);
void set_cursor(uint8_t row, uint8_t col);
void reset_cursor(void);

#endif
