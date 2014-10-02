#include "ethernet.h"
#include "arp.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>
#include <sys/socket.h>

void ethernet_handle_frame(struct network_device *dev, uint8_t *data, size_t len, size_t *reply_len)
{
	struct ethernet_header *ehdr=(struct ethernet_header*)data;
	kprintf("ETHERTYPE: %x\n", ehdr->ethertype);
	switch(htons(ehdr->ethertype))
	{
		case IPV4:
			break;
		case ARP:
			arp_handle_frame(dev, data + sizeof(struct ethernet_header), len, reply_len);
			return;
	}
}
