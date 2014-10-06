#ifndef _TAPIOS_ARP_H_
#define _TAPIOS_ARP_H_

#include <util/util.h>
#include <util/list.h>
#include "ethernet.h"

struct arp_header
{
	uint16_t hw_type;
	uint16_t protocol_type;
	uint8_t hw_size;
	uint8_t protocol_size;
	uint16_t opcode;
	uint8_t source_mac[6];
	uint32_t source_ip;
	uint8_t target_mac[6];
	uint32_t target_ip;
} __attribute__((packed));

struct arp_packet
{
	struct ethernet_header eth_header;
	struct arp_header arp_header;
} __attribute__ ((packed));

struct ip_mac_pair
{
	uint32_t ip;
	uint8_t mac[6];

	struct list list;
};

volatile struct ip_mac_pair *arp_cache;

void arp_handle_frame(struct network_device *dev, uint8_t *data, size_t len, size_t *outlen);
struct arp_packet arp_request(struct network_device *dev, uint32_t target_ip);

#endif
