#ifndef _TAPIOS_IPV4_H_
#define _TAPIOS_IPV4_H_

#include <util/util.h>

struct ipv4_header
{
	struct {
		unsigned version : 4;
		unsigned IHL	 : 4;
		unsigned DSCP	 : 6;
		unsigned ECN	 : 2;
	};
	uint16_t length;
	uint16_t identification;
	struct {
		unsigned flags : 3;
		unsigned fragment_offset : 13;
	};
	uint8_t TTL;
	uint8_t protocol;
	uint16_t header_checksum;
	uint32_t src_ip;
	uint32_t dest_ip;

	// Optional options, which usually aren't used
};

#endif
