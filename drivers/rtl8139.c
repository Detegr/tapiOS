#include "rtl8139.h"
#include <terminal/vga.h>
#include <drivers/pci.h>
#include <mem/vmm.h>

#define MAC0 0x0
#define MAR0 0x8
#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C
#define ISR 0x3E
#define RCR 0x44
#define CFG 0x52

#define RXBUF_LEN 8192+16+1500

const struct pci_device *rtl8139_device;
uint8_t rxbuf[RXBUF_LEN];

void register_rtl8139_driver(void)
{
	rtl8139_device=pci_get_device(0x10EC, 0x8139);
	if(rtl8139_device)
	{
		memset(rxbuf, 0, RXBUF_LEN);
		// Enable PCI Bus mastering by setting bit 2 in pci command register
		uint32_t cmd=pci_config_read_word(0, 4, 0, 0x4);
		pci_config_write_word(0, 4, 0, 0x6, cmd|0x4);
		uint32_t base=rtl8139_device->bar0 & 0xFFFFFFFC;
		// Power-on device by sending 0x0 to config register
		outb(base+CFG, 0x0);
		// Do a software reset by sending 0x10 to command register
		outb(base+CMD, 0x10);
		while((inb(base+CMD) & 0x10)!=0) {/*Spinloop while waiting the card to be soft resetted*/}

		// Specify the location of receive buffer (as a physical address)
		outdw(base+RBSTART, vaddr_to_physaddr((vaddr_t)rxbuf));
		// Setup interrupt mask register to generate interrupts from Transmit OK (TOK) and Receive OK (ROK)
		outdw(base+IMR, 0x5);
		// Use receive buffer wrap around, accept all packets
		outdw(base+RCR, 0x8F);
		// Allow accepting and transmitting packets
		outb(base+CMD, 0x0C);

		// TODO: Setup interrupts

		kprintf("MAC: ");
		for(int i=0; i<5; ++i)
		{
			kprintf("%d:", inb(base+MAC0+i));
		}
		kprintf("%d\n", inb(base+MAC0+5));
	}
	print_startup_info("RTL8139", rtl8139_device!=NULL);
}
