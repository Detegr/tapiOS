#include "ethernet.h"
#include <terminal/vga.h>

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

struct arp_header *arp_handle_frame(uint8_t *data, size_t len, size_t *outlen)
{
	memcpy(&arp_reply, data, sizeof(struct arp_header));
	dump_arp_header(&arp_reply);
	*outlen=0;
	return NULL;
}
