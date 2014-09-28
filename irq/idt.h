#ifndef _TAPIOS_IDT_H_
#define _TAPIOS_IDT_H_

// Interrupt types
#define GATE_INT32 0x8E
#define GATE_INT32_USER_PRIVILEGE 0xEE // 0x83 | 0x60
#define TRAP_INT32 0x8F

#include <util/util.h>

void setup_idt(void);
void idtentry(int n, uint32_t offset, uint16_t selector, uint8_t type);

#endif
