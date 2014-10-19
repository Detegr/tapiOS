#include "ipv4.h"
#include "tcp.h"
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

void ipv4_handle_frame(struct network_device *dev, struct ethernet_header *ethh, struct ipv4_header *ipvh, size_t len)
{
	if(ipvh->protocol==TCP_PROTOCOL_NUMBER)
	{
		tcp_handle_frame(dev, ethh, ipvh, (struct tcp_header*)((uint8_t*)ipvh + sizeof(struct ipv4_header)), len);
	}
}
