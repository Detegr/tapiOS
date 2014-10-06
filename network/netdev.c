#include "netdev.h"
#include "arp.h"
#include "tcp.h"
#include "socket.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>

void setup_network(void)
{
	arp_cache=NULL;
	network_devices=NULL;
	open_sockets=NULL;
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

ssize_t tx_tcp(struct file *file, const void *data, size_t count)
{
	struct network_device *dev=file->inode->device;
	struct tcp_packet tcpp;
	kprintf("Send TCP\n");
	return dev->n_act->tx(file, data, count);
}

struct inode *alloc_netdev_inode(void)
{
	struct inode *inode=vfs_new_inode(NULL, NULL, 0);
	inode->f_act=kmalloc(sizeof(struct file_actions));
	memset(inode->f_act, 0, sizeof(struct file_actions));
	inode->f_act->write=&tx_tcp;
	inode->device=network_devices;
	return inode;
}
