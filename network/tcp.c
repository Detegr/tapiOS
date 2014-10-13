#include "arp.h"
#include "tcp.h"
#include <sys/socket.h>
#include <terminal/vga.h>
#include <util/list.h>
#include <mem/kmalloc.h>
#include <util/random.h>

void tcp_dump_header(struct tcp_header *h)
{
	kprintf("FIN: %d SYN: %d ACK: %d RST: %d\n", h->opts.FIN, h->opts.SYN, h->opts.ACK, h->opts.RST);
	//kprintf("Window size: %d\nOffset: %d\nsrc port: %d\ndst port:%d\n", h->window_size, h->offset, h->port_src, h->port_dst);
}

uint16_t tcp_checksum(struct tcp_packet *p, size_t data_len)
{
	uint32_t tmp=0;
	uint16_t *ptr=(uint16_t*)&p->ipv4_header.src_ip;
	tmp+=ptr[0] + ptr[1];
	ptr=(uint16_t*)&p->ipv4_header.dest_ip;
	tmp+=ptr[0] + ptr[1];
	tmp+=htons((uint16_t)6);
	tmp+=htons((uint16_t)sizeof(struct tcp_header) + data_len);
	ptr=(uint16_t*)&p->tcp_header;
	size_t tcpsize=sizeof(struct tcp_header) + data_len;
	for(unsigned i=0; i<tcpsize/2; ++i)
	{
		tmp+=ptr[i];
	}
	uint16_t *u16ptr=(uint16_t*)&tmp;
	return ~(u16ptr[0] + u16ptr[1]);
}

void tcp_build_packet(struct network_device *dev, const uint8_t *dest_mac, const struct sockaddr_in *dest_addrin, struct tcp_packet *p, const struct tcp_opts *opts, const void *data, size_t data_len)
{
	// Ethernet header
	p->eth_header.ethertype=htons(IPV4);
	memcpy(p->eth_header.mac_src, dev->mac, 6);
	memcpy(p->eth_header.mac_dst, dest_mac, 6);

	// IP header
	p->ipv4_header.version=4; // IPV4
	p->ipv4_header.IHL=5; // TODO: Correct length (minimum is 5)
	p->ipv4_header.DSCP=0;
	p->ipv4_header.ECN=0;
	p->ipv4_header.identification=0;
	p->ipv4_header.flags=0;
	p->ipv4_header.fragment_offset=0;
	p->ipv4_header.length=htons((p->ipv4_header.IHL*4) + sizeof(struct tcp_header) + data_len); // TODO: Length
	p->ipv4_header.src_ip=htonl(MY_IP); // TODO
	p->ipv4_header.dest_ip=dest_addrin->sin_addr; // Already in network byte order
	p->ipv4_header.protocol=TCP_PROTOCOL_NUMBER;
	p->ipv4_header.TTL=64; // Recommended initial value by wikipedia
	p->ipv4_header.checksum=0;
	p->ipv4_header.checksum=ipv4_checksum(&p->ipv4_header);

	// TCP header
	memset(&p->tcp_header, 0, sizeof(struct tcp_header));
	if(opts) p->tcp_header.opts=*opts;
	p->tcp_header.port_dst=dest_addrin->sin_port;
	p->tcp_header.opts.offset=5; // Minimum
	p->tcp_header.window_size=htons(1024);
	p->tcp_header.urgent_ptr=0;
	p->tcp_header.seq_no=(uint32_t)rand();

	memcpy(p->data, data, data_len);
}

void tcp_send_packet(struct network_device *dev, struct socket *s, uint32_t seq_no, uint32_t ack_no, struct tcp_opts *opts)
{
	struct tcp_packet p;
	tcp_build_packet(dev, s->dst_mac, (const struct sockaddr_in*)&s->saddr, &p, opts, NULL, 0);
	p.tcp_header.port_src=s->src_port;
	p.tcp_header.seq_no=seq_no;
	p.tcp_header.ack_no=ack_no;
	p.tcp_header.checksum=tcp_checksum(&p, 0);
	s->seq_no=seq_no;
	s->ack_no=ack_no;

	// TODO: These fake structs are ugly :(
	struct inode i={
		.device=dev
	};
	struct file f={
		.inode=&i
	};
	dev->n_act->tx(&f, &p, sizeof(struct tcp_packet));
};

static struct socket *tcp_remove_connection(struct ipv4_header *iph, struct tcp_header *tcph)
{
	volatile struct socket *sock=NULL;
	list_foreach(open_sockets, volatile struct socket, s)
	{
		if(s->saddr.sin_addr == iph->src_ip && s->src_port == tcph->port_dst)
		{
			sock=s;
			break;
		}
	}
	if(sock)
	{
		if(sock==open_sockets)
		{
			open_sockets=(volatile struct socket*)sock->list.next;
		}
		else
		{
			list_foreach(open_sockets, volatile struct socket, s)
			{
				if(s->list.next == &sock->list)
				{
					s->list.next=sock->list.next;
					break;
				}
			}
		}
		sock->state=DISCONNECTED;
	}
	return (struct socket*)sock;
}

void tcp_handle_frame(struct network_device *dev, struct ipv4_header *iph, struct tcp_header *tcph, size_t len)
{
	tcp_dump_header(tcph);
	if(tcph->opts.RST)
	{
		struct socket *s=tcp_remove_connection(iph, tcph);
		if(s)
		{
			kprintf("Connection removed\n");
			s->refcount--;
			if(s->refcount) kprintf("Tried to free a socket that has refcount %d!\n", s->refcount);
			else kfree(s);
		}
		else kprintf("!! No connection found. Spurious reset\n");
	}
	struct socket *sock=NULL;
	list_foreach(open_sockets, volatile struct socket, s)
	{
		if(s->src_port == tcph->port_dst)
		{
			sock=(struct socket*)s;
			break;
		}
	}
	if(sock)
	{
		struct tcp_opts ack={
			.ACK=1
		};
		if(tcph->opts.SYN && tcph->opts.ACK)
		{
			uint32_t ack_no=tcph->seq_no + htonl(1U);
			tcp_send_packet(dev, sock, tcph->ack_no, ack_no, &ack);
			sock->state=CONNECTED;
		}
		else if((tcph->opts.FIN && sock->state==FIN_WAIT2) ||
				(tcph->opts.FIN && tcph->opts.ACK && sock->state==FIN_WAIT1))
		{
			uint32_t ack_no=tcph->seq_no + htonl(1U);
			tcp_send_packet(dev, sock, tcph->ack_no, ack_no, &ack);
			struct socket *s=tcp_remove_connection(iph, tcph);
			if(s)
			{
				kprintf("Connection removed\n");
				s->state=DISCONNECTED;
				s->refcount--;
				if(s->refcount) kprintf("Tried to free a socket that has refcount %d!\n", s->refcount);
				else kfree(s);
			}
			else kprintf("!! No connection found. Spurious reset\n");
		}
		else if(tcph->opts.ACK && sock->state==FIN_WAIT1)
		{
			sock->state=FIN_WAIT2;
		}
		else if(tcph->opts.ACK)
		{
			sock->seq_no=tcph->ack_no;
			sock->ack_no=tcph->seq_no;
		}
	}
}
