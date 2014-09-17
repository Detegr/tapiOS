#include <terminal/vga.h>
#include <mem/kmalloc.h>
#include "pci.h"

uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	struct pci_config c;
	c.offset=0;
	c.zero=0;
	c.enabled=1;
	c.bus_no=bus;
	c.dev_no=device;
	c.func_no=func;
	c.offset |= (offset>>2);

	uint32_t outaddr=*(uint32_t*)&c;
	outdw(CONFIG_ADDRESS, outaddr);

	return (uint16_t)(indw(CONFIG_DATA) >> ((offset & 2) * 8) & 0xFFFF);
}
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t data)
{
	struct pci_config c;
	c.offset=0;
	c.zero=0;
	c.enabled=1;
	c.bus_no=bus;
	c.dev_no=device;
	c.func_no=func;
	c.offset |= (offset>>2);

	uint32_t outaddr=*(uint32_t*)&c;
	outdw(CONFIG_ADDRESS, outaddr);
	outdw(CONFIG_DATA, data);
}

static inline uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device)
{
	return pci_config_read_word(bus, device, 0, 0);
}

static inline uint16_t pci_get_device_id(uint8_t bus, uint8_t device)
{
	return pci_config_read_word(bus, device, 0, 2);
}

static bool pci_check_device(uint8_t bus, uint8_t device, uint8_t function)
{
	return true;
}

static bool pci_device_exists(uint8_t bus, uint8_t device)
{
	uint16_t vendor=pci_get_vendor_id(bus, device);
	if(vendor == 0xFFFF) return false;
	return pci_check_device(bus, device, 0);
}

static void pci_enumerate_devices(void)
{
	for(int bus=0; bus<256; ++bus)
	{
		for(int device=0; device<32; ++device)
		{
			if(pci_device_exists(bus, device))
			{
				uint16_t vendor=pci_get_vendor_id(bus, device);
				struct pci_device *dev=pci_devices;
				if(!dev)
				{
					dev=kmalloc(sizeof(struct pci_device));
					dev->next=NULL;
					pci_devices=dev;
				}
				else
				{
					while(dev->next) dev=dev->next;
					dev->next=kmalloc(sizeof(struct pci_device));
					dev=dev->next;
					dev->next=NULL;
				}
				uint8_t header_type=pci_config_read_word(bus, device, 0, 0xD);
				if(header_type!=0x0)
				{// PCI-to-PCI bridge or CardBus bridge not supported
					kprintf("Unsupported PCI header: %x\n", header_type);
					PANIC();
				}
				dev->vendor_id=vendor;
				dev->device_id=pci_get_device_id(bus, device);
				dev->header_type=header_type;
				dev->bar0=pci_config_read_word(bus, device, 0, 0x12)<<16|pci_config_read_word(bus, device, 0, 0x10);
				dev->bar1=pci_config_read_word(bus, device, 0, 0x16)<<16|pci_config_read_word(bus, device, 0, 0x14);
				dev->bar2=pci_config_read_word(bus, device, 0, 0x1A)<<16|pci_config_read_word(bus, device, 0, 0x18);
				dev->bar3=pci_config_read_word(bus, device, 0, 0x1E)<<16|pci_config_read_word(bus, device, 0, 0x1C);
				dev->bar4=pci_config_read_word(bus, device, 0, 0x22)<<16|pci_config_read_word(bus, device, 0, 0x20);
				dev->bar5=pci_config_read_word(bus, device, 0, 0x26)<<16|pci_config_read_word(bus, device, 0, 0x24);
				dev->command=pci_config_read_word(bus, device, 0, 0x4);
				dev->status=pci_config_read_word(bus, device, 0, 0x6);
				/*
				kprintf("%x\n", dev->bar0);
				kprintf("%x\n", dev->bar1);
				kprintf("%x\n", dev->bar2);
				kprintf("%x\n", dev->bar3);
				kprintf("%x\n", dev->bar4);
				kprintf("%x\n", dev->bar5);*/
				kprintf("%s %@[%@Vendor: %@%x%@ Device: %@%x%@]\n",
						"PCI device found",
						0x0F, 0x07, 0x09, vendor, 0x07, 0x09, pci_get_device_id(bus, device), 0x0F);
			}
		}
	}
}

const struct pci_device *pci_get_device(uint32_t vendor, uint32_t device)
{
	const struct pci_device *dev=pci_devices;
	while(dev)
	{
		if(dev->vendor_id==vendor && dev->device_id==device) return dev;
		dev=dev->next;
	}
	return NULL;
}

void register_pci_driver(void)
{
	pci_devices=NULL;
	pci_enumerate_devices();
	print_startup_info("PCI", true);
}
