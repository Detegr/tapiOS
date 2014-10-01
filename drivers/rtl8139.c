#include "rtl8139.h"
#include <irq/irq.h>
#include <terminal/vga.h>
#include <dev/pci.h>
#include <mem/vmm.h>
#include <mem/kmalloc.h>
#include <network/ethernet.h>
#include <network/netdev.h>

#define MAC0 0x0
#define MAR0 0x8
#define TXSTAT 0x10
#define TXBUF 0x20
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

#define ROK 1
#define TOK 4

static ssize_t rtl8139_tx(struct file *f, const void *data, uint32_t len)
{
	struct network_device *rtl=f->inode->device;
	if(!rtl)
	{
		kprintf("Could not find network device from inode!\n");
		PANIC();
	}
	if(data==NULL) return -1;
	memcpy(rtl->tx_buf[rtl->tx_pos], data, len);
	if(len<60)
	{// Pad with zeros until the length is long enough
		memset(rtl->tx_buf[rtl->tx_pos] + len, 0, 60-len);
		len=60;
	}
	outdw(rtl->io_base+TXBUF + (4*rtl->tx_pos), vaddr_to_physaddr((vaddr_t)rtl->tx_buf[rtl->tx_pos]));
	// Clears the OWN bit and sets the length,
	// sets the early transmit treshold to 8 bytes
	outdw(rtl->io_base+TXSTAT + (4*rtl->tx_pos), len & 0xFFF);
	rtl->tx_pos = (rtl->tx_pos + 1) % 4;
	rtl->tx_buffers_free--;

	return len;
}

static void rtl8139_isr(void *rtl_dev, struct registers *regs)
{
	struct network_device *rtl=(struct network_device*)rtl_dev;

	uint16_t status=inw(rtl->io_base+ISR);
	outdw(rtl->io_base+ISR, status);
	if(status & ROK)
	{
		while((inw(rtl->io_base+CMD) & RXBUF_EMPTY) == 0)
		{
			kprintf("rtl status: %d, pos %d\n", status, rtl->rx_pos);
			kprintf("status from packet: %d\n", *(uint16_t*)(rtl->rx_buf + rtl->rx_pos));
			uint16_t rx_len=*(uint16_t*)(rtl->rx_buf + rtl->rx_pos + 2);
			kprintf("packet length: %d\n", rx_len);

			// Handle the packet and send reply if needed
			size_t reply_len;
			uint8_t *data=rtl->rx_buf + rtl->rx_pos + 4;
			ethernet_handle_frame(rtl, data, rx_len, &reply_len);

			// Update CAPR. This is some higher level magic found from the manual
			// +4 is the header, +3 is dword alignment
			rtl->rx_pos=(rtl->rx_pos + rx_len + 4 + 3) & RX_READ_POINTER_MASK;
			outw(rtl->io_base + CAPR, rtl->rx_pos - 0x10);
			rtl->rx_pos%=0x2000;
		}
	}
	if(status & TOK)
	{
		kprintf("Transmit OK\n");
	}
}

static void populate_netdev(struct pci_device *pdev, struct network_device *dev)
{
	dev->n_act=kmalloc(sizeof(struct network_actions));
	dev->n_act->tx=&rtl8139_tx;

	dev->io_base=pci_config_read_dword(pdev->bus_id, pdev->bus_device_id, 0, PCI_BAR0) & 0xFFFFFFFC;
	dev->irq=pci_config_read_word(pdev->bus_id, pdev->bus_device_id, 0, PCI_INTERRUPT_LINE);
	dev->rx_buf=kmalloc(RXBUF_LEN+MTU+16);
	for(int i=0; i<4; ++i)
	{
		dev->tx_buf[i]=kmalloc(2048);
	}
	dev->tx_pos=0;
	dev->tx_buffers_free=4;
	dev->rx_pos=0;

	for(int i=0; i<6; ++i)
	{
		dev->mac[i]=inb(dev->io_base+MAC0+i);
	}
}

void register_rtl8139_driver(void)
{
	struct pci_device *rtl_pdev=pci_get_device(0x10EC, 0x8139);
	if(rtl_pdev)
	{
		struct network_device *rtl=kmalloc(sizeof(struct network_device));
		populate_netdev(rtl_pdev, rtl);
		memset(rtl->rx_buf, 0, RXBUF_LEN+MTU+16);
		kprintf("RTL8139 IRQ: %d\n", rtl->irq);

		// Enable PCI Bus mastering by setting bit 2 in pci command register
		uint16_t cmd=pci_config_read_word(rtl_pdev->bus_id, rtl_pdev->bus_device_id, 0, PCI_CMD);
		pci_config_write_word(rtl_pdev->bus_id, rtl_pdev->bus_device_id, 0, PCI_CMD, cmd|0x4);

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

		register_isr(rtl->irq, &rtl8139_isr, rtl);
		register_network_device(rtl);
	}
	print_startup_info("RTL8139", rtl_pdev!=NULL);
}
