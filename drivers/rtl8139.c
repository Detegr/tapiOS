#include "rtl8139.h"
#include <terminal/vga.h>
#include <dev/pci.h>
#include <mem/vmm.h>
#include <mem/kmalloc.h>

#define MAC0 0x0
#define MAR0 0x8
#define RBSTART 0x30
#define CMD 0x37
#define CAPR 0x38
#define IMR 0x3C
#define ISR 0x3E
#define RCR 0x44
#define CFG 0x52

#define RXBUF_EMPTY 0x1
#define RX_READ_POINTER_MASK (~3)

#define RXBUF_LEN 8192
#define MTU 1500
#define TXBUF_LEN RXBUF_LEN

struct rtl8139
{
	uint8_t bus;
	uint8_t device;
	uint32_t io_base;
	uint8_t irq;
	uint8_t mac[6];

	uint8_t *rx_buf;
	uint16_t rx_pos;
	uint8_t *tx_buf;
};

static void populate_rtl8139(struct pci_device *pdev, struct rtl8139 *dev)
{
	dev->bus=pdev->bus_id;
	dev->device=pdev->bus_device_id;
	dev->io_base=pci_config_read_dword(dev->bus, dev->device, 0, PCI_BAR0) & 0xFFFFFFFC;
	dev->irq=pci_config_read_word(dev->bus, dev->device, 0, PCI_INTERRUPT_LINE);
	dev->rx_buf=kmalloc(RXBUF_LEN+MTU+16);
	dev->tx_buf=kmalloc(TXBUF_LEN);
	dev->rx_pos=0;
	for(int i=0; i<6; ++i)
	{
		dev->mac[i]=inb(dev->io_base+MAC0+i);
	}
}

static void dump_mac_addr(struct rtl8139 *dev)
{
	kprintf("MAC: ");
	for(int i=0; i<5; ++i)
	{
		kprintf("%X:", dev->mac[i]);
	}
	kprintf("%X\n", dev->mac[5]);
}

void register_rtl8139_driver(void)
{
	struct pci_device *rtl_pdev=pci_get_device(0x10EC, 0x8139);
	if(rtl_pdev)
	{
		struct rtl8139 *rtl=kmalloc(sizeof(struct rtl8139));
		populate_rtl8139(rtl_pdev, rtl);
		rtl_pdev->device=rtl;
		memset(rtl->rx_buf, 0, RXBUF_LEN+MTU+16);
		kprintf("RTL8139 IRQ: %d\n", rtl->irq);

		// Enable PCI Bus mastering by setting bit 2 in pci command register
		uint16_t cmd=pci_config_read_word(rtl->bus, rtl->device, 0, PCI_CMD);
		pci_config_write_word(rtl->bus, rtl->device, 0, PCI_CMD, cmd|0x4);

		// Power-on device by sending 0x0 to config register
		outb(rtl->io_base+CFG, 0x0);

		// Do a software reset by sending 0x10 to command register
		outb(rtl->io_base+CMD, 0x10);
		while((inb(rtl->io_base+CMD) & 0x10)!=0) {/*Spinloop while waiting the card to be soft resetted*/}

		// Specify the location of receive buffer (as a physical address)
		outdw(rtl->io_base+RBSTART, vaddr_to_physaddr((vaddr_t)rtl->rx_buf));

		// Setup interrupt mask register to generate interrupts from Transmit OK (TOK) and Receive OK (ROK)
		outw(rtl->io_base+IMR, 0x5);

		// Do not use ring buffer style receive buffer, accept all packets
		outdw(rtl->io_base+RCR, 0x8F);

		// Allow accepting and transmitting packets
		outb(rtl->io_base+CMD, 0x0C);

		dump_mac_addr(rtl);
	}
	print_startup_info("RTL8139", rtl_pdev!=NULL);
}

void irq11_handler(void)
{
	struct rtl8139 *rtl=pci_get_device(0x10EC, 0x8139)->device; // TODO: How to do multiple devices?
	uint16_t status=inw(rtl->io_base+ISR);
	outdw(rtl->io_base+ISR, status);
	while((inw(rtl->io_base+CMD) & RXBUF_EMPTY) == 0)
	{
		kprintf("rtl status: %d, pos %d\n", status, rtl->rx_pos);
		kprintf("status from packet: %d\n", *(uint16_t*)(rtl->rx_buf + rtl->rx_pos));
		uint16_t rx_len=*(uint16_t*)(rtl->rx_buf + rtl->rx_pos + 2);
		kprintf("packet length: %d\n", rx_len);
		for(int i=rtl->rx_pos; i<rtl->rx_pos+rx_len; i+=2)
		{
			kprintf("%X ", *(uint8_t*)(rtl->rx_buf+i+1));
			kprintf("%X ", *(uint8_t*)(rtl->rx_buf+i));
		}
		kprintf("\n");

		// Update CAPR. This is some higher level magic found from the manual
		// +4 is the header, +3 is dword alignment
		rtl->rx_pos=(rtl->rx_pos + rx_len + 4 + 3) & RX_READ_POINTER_MASK;
		outw(rtl->io_base + CAPR, rtl->rx_pos - 0x10);
		rtl->rx_pos%=0x2000;
	}
}
