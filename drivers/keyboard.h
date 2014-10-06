#ifndef _TAPIOS_DRIVERS_KEYBOARD_H_
#define _TAPIOS_DRIVERS_KEYBOARD_H_

#include <stdint.h>
#include <task/process.h>

void register_kbd_driver(void);
void kbd_buffer_push(uint8_t c);
ssize_t kbd_read(struct file *f, void *top, size_t count);
bool kbd_hasdata(void);

#endif
