#ifndef _TAPIOS_VIDEO_H_
#define _TAPIOS_VIDEO_H_

#include <stdint.h>
#include <stdbool.h>

volatile unsigned char* video;

void cls(void);
void cls_from_cursor_down(void);
void print_startup_info(const char* section, bool ok);
void kprintf(const char* fmt, ...);
void kprintc(const char c);
void hide_cursor(void);
void set_cursor(uint8_t row, uint8_t col);
void move_cursor(uint8_t rows, uint8_t cols);
void update_cursor(void);
void reset_cursor(void);
void delete_last_char(int,int);
int get_cursor_row(void);
int get_cursor_col(void);

#endif
