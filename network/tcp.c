#include "tcp.h"
#include <sys/socket.h>
#include <terminal/vga.h>

uint16_t tcp_checksum(struct tcp_packet *p)
{
	uint32_t tmp=0;
	uint16_t *ptr=(uint16_t*)&p->ipv4_header.src_ip;
	tmp+=ptr[0] + ptr[1];
	ptr=(uint16_t*)&p->ipv4_header.dest_ip;
	tmp+=ptr[0] + ptr[1];
	tmp+=htons((uint16_t)6);
	tmp+=htons((uint16_t)sizeof(struct tcp_header)); // TODO: + sizeof tcp data
	ptr=(uint16_t*)&p->tcp_header;
	size_t tcpsize=sizeof(struct tcp_header);
	for(unsigned i=0; i<tcpsize/2; ++i)
	{
		tmp+=ptr[i];
	}
	// TODO: Actual tcp data
	uint16_t *u16ptr=(uint16_t*)&tmp;
	return ~(u16ptr[0] + u16ptr[1]);
}

void build_tcp_packet(struct network_device *dev, uint8_t *dest_mac, struct sockaddr_in *dest_addrin, struct tcp_packet *p, void *data)
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
	p->ipv4_header.length=htons((p->ipv4_header.IHL*4) + sizeof(struct tcp_header)); // TODO: Length
	p->ipv4_header.src_ip=htonl(MY_IP); // TODO
	p->ipv4_header.dest_ip=dest_addrin->sin_addr; // Already in network byte order
	p->ipv4_header.protocol=TCP_PROTOCOL_NUMBER;
	p->ipv4_header.TTL=64; // Recommended initial value by wikipedia
	p->ipv4_header.checksum=0;
	p->ipv4_header.checksum=ipv4_checksum(&p->ipv4_header);

	// TCP header
	memset(&p->tcp_header, 0, sizeof(struct tcp_header));
	p->tcp_header.port_src=htons(1000); // TODO
	p->tcp_header.port_dst=dest_addrin->sin_port;
	p->tcp_header.SYN=1;
	p->tcp_header.seq_no=0;
	p->tcp_header.ack_no=0;
	p->tcp_header.offset=5; // Minimum
	p->tcp_header.window_size=htons(1024);
	p->tcp_header.urgent_ptr=0;
	p->tcp_header.checksum=tcp_checksum(p);
}
