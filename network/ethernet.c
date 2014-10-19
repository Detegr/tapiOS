#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>
#include <sys/socket.h>

void ethernet_handle_frame(struct network_device *dev, uint8_t *data, size_t len)
{
	struct ethernet_header *ehdr=(struct ethernet_header*)data;
	switch(htons(ehdr->ethertype))
	{
		case IPV4:
			ipv4_handle_frame(dev, (struct ethernet_header*)data, (struct ipv4_header*)(data + sizeof(struct ethernet_header)), len);
			return;
		case ARP:
			arp_handle_frame(dev, data + sizeof(struct ethernet_header), len);
			return;
	}
}
