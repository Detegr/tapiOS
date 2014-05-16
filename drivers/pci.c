#include <terminal/vga.h>
#include "pci.h"

static uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
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

static inline uint16_t pci_get_vendor(uint8_t bus, uint8_t device)
{
	return pci_config_read_word(bus, device, 0, 0);
}

static inline uint16_t pci_get_device(uint8_t bus, uint8_t device)
{
	return pci_config_read_word(bus, device, 0, 2);
}

static bool pci_check_device(uint8_t bus, uint8_t device, uint8_t  function)
{
	return true;
}

static bool pci_device_exists(uint8_t bus, uint8_t device)
{
	uint16_t vendor=pci_get_vendor(bus, device);
	if(vendor == 0xFFFF) return false;
	if(pci_check_device(bus, device, 0))
	{
		kprintf("%s %@[%@Vendor: %@%x%@ Device: %@%x%@]\n",
				"PCI device found",
				0x0F, 0x07, 0x09, vendor, 0x07, 0x09, pci_get_device(bus, device), 0x0F);
		return true;
	}
	return false;
}

static void pci_enumerate_devices(void)
{
	for(int bus=0; bus<256; ++bus)
	{
		for(int device=0; device<32; ++device)
		{
			pci_device_exists(bus, device);
		}
	}
}

void register_pci_driver(void)
{
	pci_enumerate_devices();
	print_startup_info("PCI", true);
}
