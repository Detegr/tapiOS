#include "ipv4.h"
#include <sys/socket.h>
#include <terminal/vga.h>

uint16_t ipv4_checksum(struct ipv4_header *h)
{
	uint32_t tmp=0;
	uint16_t *ptr=(uint16_t*)h;
	for(int i=0; i<10; ++i)
	{
		tmp += ptr[i];
	}
	uint16_t *u16ptr=(uint16_t*)&tmp;
	return ~(u16ptr[0] + u16ptr[1]);
}
