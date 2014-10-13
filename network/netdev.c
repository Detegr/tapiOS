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
	struct socket *s=(struct socket*)file;
	uint8_t buf[sizeof(struct tcp_packet) + count];
	struct tcp_packet *p=(struct tcp_packet*)&buf;
	struct tcp_opts opts={
		.ACK=1
	};
	tcp_build_packet(dev, s->dst_mac, &s->saddr, p, &opts, data, count);
	p->tcp_header.port_src=s->src_port;
	p->tcp_header.seq_no=s->seq_no;
	p->tcp_header.ack_no=s->ack_no;
	p->tcp_header.checksum=tcp_checksum(p, count);
	s->seq_no=p->tcp_header.seq_no + htonl(count);
	s->ack_no=p->tcp_header.ack_no;
	return dev->n_act->tx(file, buf, sizeof(buf));
}

int32_t netdev_close(struct file *f)
{
	struct socket *s=(struct socket*)f;
	if(s->state==CONNECTED)
	{
		struct tcp_opts opts={
			.FIN=1,
			.ACK=1
		};
		s->state=FIN_WAIT1;
		tcp_send_packet(s->inode->device, s, s->seq_no, s->ack_no, &opts);
	}
	return 0;
}

struct inode *alloc_netdev_inode(void)
{
	struct inode *inode=vfs_new_inode(NULL, NULL, 0);
	inode->f_act=kmalloc(sizeof(struct file_actions));
	memset(inode->f_act, 0, sizeof(struct file_actions));
	inode->f_act->write=&tx_tcp;
	inode->f_act->close=&netdev_close;
	inode->device=network_devices;
	return inode;
}
