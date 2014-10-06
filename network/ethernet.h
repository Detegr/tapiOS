#ifndef _TAPIOS_ETHERNET_H_
#define _TAPIOS_ETHERNET_H_

#include <util/util.h>
#include <network/netdev.h>

// Ethertypes
#define IPV4 0x0800
#define ARP 0x0806

// TODO: Read these from a config file of some sort
#define MY_IP 0x0A000002
#define SUBNET_MASK 0xFFFFFF00
#define DEFAULT_GW 0x0A000001

#define ARP_REQUEST 0x1
#define ARP_REPLY 0x2

struct ethernet_header
{
	uint8_t mac_dst[6];
	uint8_t mac_src[6];
	uint16_t ethertype;
} __attribute__((packed));

void ethernet_handle_frame(struct network_device *dev, uint8_t *data, size_t len);

#endif
