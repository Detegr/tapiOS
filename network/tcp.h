#ifndef _TAPIOS_TCP_H_
#define _TAPIOS_TCP_H_

#include <util/util.h>
#include "ethernet.h"
#include "ipv4.h"

struct tcp_header
{
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t seq_no;
	uint32_t ack_no;
	uint8_t offset;
	struct {
		unsigned RESERVED	: 3;
		unsigned NS 		: 1;
		unsigned CWR		: 1;
		unsigned ECE		: 1;
		unsigned URG		: 1;
		unsigned ACK		: 1;
		unsigned PSH		: 1;
		unsigned RST		: 1;
		unsigned SYN		: 1;
		unsigned FIN		: 1;
	} __attribute__((packed));
	uint16_t checksum;
	uint16_t urgent_ptr;

	// Options (0-40 bytes) are optional and must be aligned by 32 bits
} __attribute__((aligned(4)));

struct tcp_packet
{
	struct ethernet_header eth_header;
	struct ipv4_header ipv4_header;
	struct tcp_header tcp_header;
};

#endif
