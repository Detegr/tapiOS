#ifndef _TAPIOS_GDT_H_
#define _TAPIOS_GDT_H_

// GDT selectors
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

void setup_gdt(void);

#endif
