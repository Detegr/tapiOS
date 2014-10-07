#ifndef _TAPIOS_TCP_H_
#define _TAPIOS_TCP_H_

#include <util/util.h>
#include <sys/socket.h>
#include "ethernet.h"
#include "ipv4.h"

#define TCP_PROTOCOL_NUMBER 6

struct tcp_header
{
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t seq_no;
	uint32_t ack_no;
	struct {
		unsigned RESERVED	: 3;
		unsigned NS 		: 1;
		unsigned offset     : 4;
		unsigned FIN		: 1;
		unsigned SYN		: 1;
		unsigned RST		: 1;
		unsigned PSH		: 1;
		unsigned ACK		: 1;
		unsigned URG		: 1;
		unsigned ECE		: 1;
		unsigned CWR		: 1;
	} __attribute__((packed));
	uint16_t window_size;
	uint16_t checksum;
	uint16_t urgent_ptr;

	// Options (0-40 bytes) are optional and must be aligned by 32 bits
} __attribute__((packed));

struct tcp_packet
{
	struct ethernet_header eth_header;
	struct ipv4_header ipv4_header;
	struct tcp_header tcp_header;
} __attribute__((packed));

uint16_t tcp_checksum(struct tcp_packet *p);
void tcp_build_packet(struct network_device *dev, const uint8_t *dest_mac, const struct sockaddr_in *dest_addrin, struct tcp_packet *p, const void *data);
void tcp_handle_frame(struct network_device *dev, struct ipv4_header *ipv4h, struct tcp_header *tcph, size_t len);

#endif
