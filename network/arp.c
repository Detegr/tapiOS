#include "arp.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>
#include <sys/socket.h>

static void dump_arp_header(struct arp_header *arp)
{
	kprintf("HW type: %x\n", arp->hw_type);
	kprintf("Protocol: %x\n", arp->protocol_type);
	kprintf("HW size: %d\n", arp->hw_size);
	kprintf("Protocol size: %d\n", arp->protocol_size);
	kprintf("Opcode: %x\n", arp->opcode);
	kprintf("source IP: %x\n", arp->source_ip);
	kprintf("Target IP: %x\n", arp->target_ip);
}

void arp_handle_frame(struct network_device *dev, uint8_t *data, size_t len)
{
	struct arp_packet p;
	uint32_t tmp_ip;
	memcpy(&p.arp_header, data, sizeof(struct arp_header));
	//dump_arp_header(&p.arp_header);
	if(p.arp_header.opcode == htons(ARP_REQUEST) && p.arp_header.target_ip == htonl(MY_IP))
	{
		p.eth_header.ethertype=htons(ARP);
		memcpy(p.arp_header.target_mac, p.arp_header.source_mac, 6);
		memcpy(p.arp_header.source_mac, dev->mac, 6);
		tmp_ip=p.arp_header.source_ip;
		p.arp_header.source_ip=p.arp_header.target_ip;
		p.arp_header.target_ip=tmp_ip;
		p.arp_header.opcode=htons(ARP_REPLY);

		memcpy(p.eth_header.mac_src, p.arp_header.source_mac, 6);
		memcpy(p.eth_header.mac_dst, p.arp_header.target_mac, 6);

		struct inode i={
			.device=dev
		};
		struct file f={
			.inode=&i
		};
		dev->n_act->tx(&f, &p, sizeof(struct arp_packet));
	}
	else if(p.arp_header.opcode == htons(ARP_REPLY) && p.arp_header.target_ip == htonl(MY_IP))
	{
		struct ip_mac_pair *e=NULL;
		if(!arp_cache)
		{
			e=kmalloc(sizeof(struct ip_mac_pair));
			arp_cache=e;
		}
		else
		{
			list_foreach(arp_cache, volatile struct ip_mac_pair, entry)
			{
				if(entry->ip == p.arp_header.source_ip) return;
			}
		}
		if(!e)
		{
			struct ip_mac_pair *arpc=(struct ip_mac_pair*)arp_cache;
			e=kmalloc(sizeof(struct ip_mac_pair));
			list_add(arpc, e);
		}
		e->ip=p.arp_header.source_ip;
		memcpy(e->mac, p.arp_header.source_mac, 6);
		kprintf("Added IP %d to arp cache\n", e->ip);
	}
}

struct arp_packet arp_request(struct network_device *dev, uint32_t target_ip)
{
	struct arp_packet ret;
	ret.eth_header.ethertype=htons(ARP);
	memcpy(ret.eth_header.mac_src, dev->mac, 6);
	memset(ret.eth_header.mac_dst, 0xFF, 6); // Broadcast
	ret.arp_header.hw_type=htons(1); // Ethernet
	ret.arp_header.hw_size=6;
	ret.arp_header.protocol_type=htons(IPV4);
	ret.arp_header.protocol_size=4;
	ret.arp_header.opcode=htons(ARP_REQUEST);
	ret.arp_header.target_ip=target_ip;
	ret.arp_header.source_ip=htonl(MY_IP); // TODO
	return ret;
}

bool arp_find_mac(uint32_t dest_ip, uint8_t *to)
{// Assuming that 'to' has at least 6 bytes of space
	bool ret=false;
	list_foreach(arp_cache, volatile struct ip_mac_pair, entry)
	{
		if(entry->ip == dest_ip)
		{
			memcpy(to, (const void*)entry->mac, 6);
			ret=true;
			break;
		}
	}
	return ret;
}
