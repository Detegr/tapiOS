#ifndef _TAPIOS_IRQ_H_
#define _TAPIOS_IRQ_H_

#include "util.h"
#include "vga.h"

void irq1_handler(void);

void setup_idt(void);
void remap_pic(uint8_t offset1, uint8_t offset2);

uint8_t pic_get_irq(void);
uint8_t pic1_get_isr(void);
uint8_t pic2_get_isr(void);
uint8_t is_spurious_irq_master(void);
uint8_t is_spurious_irq_slave(void);

#endif
