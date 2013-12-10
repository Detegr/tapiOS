#ifndef _TAPIOS_VIDEO_H_
#define _TAPIOS_VIDEO_H_

#include <stdint.h>

volatile unsigned char* video;

void cls(void);
void print_startup_info(const char* section, const char* msg);
void kprintf(const char* fmt, ...);
void kprintc(const char c);
void hide_cursor(void);
void set_cursor(uint8_t row, uint8_t col);
void update_cursor(void);
void reset_cursor(void);
void delete_last_char(void);

#endif
