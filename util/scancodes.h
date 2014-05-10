#ifndef _TAPIOS_SCANCODES_H_
#define _TAPIOS_SCANCODES_H_

#include <stdint.h>

#define CHAR_UNHANDLED 0
#define CHAR_UP 1
#define CHAR_BACKSPACE 8

char char_for_scancode(uint8_t scancode);

#endif
