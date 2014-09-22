#ifndef _TAPIOS_ETHERNET_H_
#define _TAPIOS_ETHERNET_H_

#include <util/util.h>

#define IPV4 0x8000
#define ARP 0x0608

struct ethernet_header
{
	uint8_t mac_dst[6];
	uint8_t mac_src[6];
	uint16_t ethertype;
} __attribute__((packed));

struct arp_header
{
	uint16_t hw_type;
	uint16_t protocol_type;
	uint8_t hw_size;
	uint8_t protocol_size;
	uint16_t opcode;
	uint8_t sender_mac[6];
	uint32_t sender_ip;
	uint8_t target_mac[6];
	uint32_t target_ip;
} __attribute__((packed));

struct arp_header arp_reply;
struct arp_header *arp_handle_frame(uint8_t *data, size_t len, size_t *outlen);

#endif
