#ifndef _TAPIOS_PIC_H_
#define _TAPIOS_PIC_H_

#include <stdint.h>

#define PIC1 0x20
#define PIC1_DATA 0x21
#define PIC2 0xA0
#define PIC2_DATA 0xA1

void remap_pic(uint8_t offset1, uint8_t offset2);
uint8_t pic_get_irq(void);
void setup_pic(void);
void timer_handler(void);

#endif
