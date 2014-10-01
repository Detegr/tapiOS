#ifndef _TAPIOS_ETHERNET_H_
#define _TAPIOS_ETHERNET_H_

#include <util/util.h>
#include <network/netdev.h>

#define IPV4 0x0800
#define ARP 0x0608

// TODO: Read these from a config file of some sort
#define MY_IP 0x200000A
#define SUBNET_MASK 0x00FFFFFF
#define DEFAULT_GW 0x100000A

#define ARP_REQUEST 0x0100
#define ARP_REPLY 0x0200

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

struct arp_packet
{
	struct ethernet_header eth_header;
	struct arp_header arp_header;
} __attribute__ ((packed));

void ethernet_handle_frame(struct network_device *dev, uint8_t *data, size_t len, size_t *reply_len);

#endif
