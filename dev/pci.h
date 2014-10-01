#ifndef _TAPIOS_DRIVERS_PCI_
#define _TAPIOS_DRIVERS_PCI_

#include <util/util.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24

#define PCI_CMD 0x4
#define PCI_STATUS 0x6
#define PCI_INTERRUPT_LINE 0x3C

struct pci_config
{
	union
	{
		struct
		{
			int zero     : 2;
			int reg_no   : 6;
			int func_no  : 3;
			int dev_no   : 5;
			int bus_no   : 8;
			int reserved : 7;
			int enabled  : 1;
		} __attribute__((packed));
		struct
		{
			int        : 2;
			int offset : 30;
		} __attribute__((packed));
	};
};

struct pci_device
{
	uint16_t device_id;
	uint16_t vendor_id;
	uint8_t bus_id;
	uint8_t bus_device_id;
	struct pci_device *next;
} __attribute__((packed));

void pci_init(void);
struct pci_device *pci_get_device(uint32_t vendor, uint32_t device);
struct pci_device *pci_devices;
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t data);

#endif
