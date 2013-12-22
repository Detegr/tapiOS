#include "timer.h"
#include <util/util.h>

#define PIT_COMMAND_PORT 0x43
#define PIT_DATA_PORT 0x40

void set_timer_freq(uint8_t freq)
{
	uint32_t div = 1193180 / freq;

	outb(0x43, 0x36); // Set repeating mode

	// Set the frequency
	outb(PIT_DATA_PORT, div & 0xFF);
	outb(PIT_DATA_PORT, (div>>8) & 0xFF);
}
