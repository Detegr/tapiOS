#include "netdev.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>

void setup_network(void)
{
	network_devices=NULL;
	print_startup_info("Network", 1);
}

void register_network_device(struct network_device *newdev)
{
	if(!network_devices)
	{
		network_devices=newdev;
	}
	else
	{
		struct network_device *dev=network_devices;
		while(dev->next) dev=dev->next;
		dev->next=newdev;
	}
}

void dump_mac_addr(struct network_device *dev)
{
	kprintf("MAC: ");
	for(int i=0; i<5; ++i)
	{
		kprintf("%X:", dev->mac[i]);
	}
	kprintf("%X\n", dev->mac[5]);
}

