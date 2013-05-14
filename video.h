#ifndef _TAPIOS_VIDEO_H_
#define _TAPIOS_VIDEO_H_

#include <stdint.h>

volatile unsigned char* video;

void cls(void);
inline void printk(const char* str);
void printkc(const char* str, uint8_t color);

#endif
