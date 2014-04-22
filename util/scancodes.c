#include "scancodes.h"
#include <stdbool.h>

char char_for_scancode(uint8_t scancode)
{
	static bool shift=false;
	char ret=0;
	switch(scancode)
	{ // Scan code set 1
		case 0x1E: ret='a'; break;
		case 0x30: ret='b'; break;
		case 0x2E: ret='c'; break;
		case 0x20: ret='d'; break;
		case 0x12: ret='e'; break;
		case 0x21: ret='f'; break;
		case 0x22: ret='g'; break;
		case 0x23: ret='h'; break;
		case 0x17: ret='i'; break;
		case 0x24: ret='j'; break;
		case 0x25: ret='k'; break;
		case 0x26: ret='l'; break;
		case 0x32: ret='m'; break;
		case 0x31: ret='n'; break;
		case 0x18: ret='o'; break;
		case 0x19: ret='p'; break;
		case 0x10: ret='q'; break;
		case 0x13: ret='r'; break;
		case 0x1F: ret='s'; break;
		case 0x14: ret='t'; break;
		case 0x16: ret='u'; break;
		case 0x2F: ret='v'; break;
		case 0x11: ret='w'; break;
		case 0x2D: ret='x'; break;
		case 0x15: ret='y'; break;
		case 0x2C: ret='z'; break;
		case 0x27: return shift ? ':' : ';';
		case 0x33: return ',';
		case 0x34: return '.';
		case 0x35: return '/';
		case 0x39: return ' ';
		case 0x1C: return '\n';
		case 0x9E:
		case 0xB0:
		case 0xAE:
		case 0xA0:
		case 0x92:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0x97:
		case 0xA4:
		case 0xA5:
		case 0xA6:
		case 0xB2:
		case 0xB1:
		case 0x98:
		case 0x99:
		case 0x90:
		case 0x93:
		case 0x9F:
		case 0x94:
		case 0x96:
		case 0xAF:
		case 0x91:
		case 0xAD:
		case 0x95:
		case 0xAC:
		case 0xA7:
		case 0xB3:
		case 0xB4:
		case 0xB9:
		case 0x9C:
			return CHAR_UP;
		// Has a bug with both shifts down at the same time, but who cares :)
		case 0x36: case 0x2A: shift=true; return CHAR_UP;
		case 0xB6: case 0xAA: shift=false; return CHAR_UP;
		case 0x3A: shift=!shift; return CHAR_UP; // Capslock down
		case 0xBA: return CHAR_UP; // Capslock up
		case 0x0E: return CHAR_BACKSPACE;
		case 0x8E: return CHAR_UP; // Backspace up
		default: return CHAR_UNHANDLED; break;
	}
	if(shift) ret-=32;
	return ret;
}
