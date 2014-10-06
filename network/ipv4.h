#ifndef _TAPIOS_IPV4_H_
#define _TAPIOS_IPV4_H_

#include <util/util.h>
#include "netdev.h"

struct ipv4_header
{
	struct {
		unsigned IHL	 : 4;
		unsigned version : 4;
		unsigned ECN	 : 2;
		unsigned DSCP	 : 6;
	} __attribute__((packed));
	uint16_t length;
	uint16_t identification;
	struct {
		unsigned flags : 3;
		unsigned fragment_offset : 13;
	} __attribute__((packed));
	uint8_t TTL;
	uint8_t protocol;
	uint16_t checksum;
	uint32_t src_ip;
	uint32_t dest_ip;

	// Optional options, which usually aren't used
} __attribute__((packed));

uint16_t ipv4_checksum(struct ipv4_header *h);
void ipv4_handle_frame(struct network_device *dev, uint8_t *data, size_t len);

#endif
