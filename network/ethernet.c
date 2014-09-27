#include "ethernet.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>

void *ethernet_handle_frame(uint8_t *data, size_t len, size_t *reply_len)
{
	struct ethernet_header *ehdr=(struct ethernet_header*)data;
	switch(ehdr->ethertype)
	{
		case IPV4:
			break;
		case ARP:
			return arp_handle_frame(data + sizeof(struct ethernet_header), len, reply_len);
	}
	return NULL;
}

static void dump_arp_header(struct arp_header *arp)
{
	kprintf("HW type: %x\n", arp->hw_type);
	kprintf("Protocol: %x\n", arp->protocol_type);
	kprintf("HW size: %d\n", arp->hw_size);
	kprintf("Protocol size: %d\n", arp->protocol_size);
	kprintf("Opcode: %x\n", arp->opcode);
	kprintf("Sender IP: %x\n", arp->sender_ip);
	kprintf("Target IP: %x\n", arp->target_ip);
}

struct arp_packet *arp_handle_frame(uint8_t *data, size_t len, size_t *outlen)
{
	struct arp_packet *reply=kmalloc(sizeof(struct arp_packet)); // TODO: Is this bad?
	uint32_t tmp_ip;
	memcpy(&reply->arp_header, data, sizeof(struct arp_header));
	dump_arp_header(&reply->arp_header);
	if(reply->arp_header.target_ip == MY_IP)
	{
		reply->eth_header.ethertype=ARP;
		memcpy(reply->arp_header.target_mac, reply->arp_header.sender_mac, 6);
		// TODO
		reply->arp_header.sender_mac[0]=0x52;
		reply->arp_header.sender_mac[1]=0x54;
		reply->arp_header.sender_mac[2]=0x00;
		reply->arp_header.sender_mac[3]=0x12;
		reply->arp_header.sender_mac[4]=0x34;
		reply->arp_header.sender_mac[5]=0x56;
		tmp_ip=reply->arp_header.sender_ip;
		reply->arp_header.sender_ip=reply->arp_header.target_ip;
		reply->arp_header.target_ip=tmp_ip;
		reply->arp_header.opcode=ARP_REPLY;
		*outlen=sizeof(struct arp_packet);

		memcpy(reply->eth_header.mac_src, reply->arp_header.sender_mac, 6);
		memcpy(reply->eth_header.mac_dst, reply->arp_header.target_mac, 6);
		return reply;
	}
	return NULL;
}
