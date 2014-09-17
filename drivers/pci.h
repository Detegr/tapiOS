#ifndef TAPIOS_DRIVERS_PCI
#define TAPIOS_DRIVERS_PCI

#include <util/util.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

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
	uint16_t status;
	uint16_t command;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t revision_id;
	uint8_t bist;
	uint8_t header_type;
	uint8_t latency_timer;
	uint8_t cache_line_size;
	union {
		struct
		{
			uint32_t bar0;
			uint32_t bar1;
			uint32_t bar2;
			uint32_t bar3;
			uint32_t bar4;
			uint32_t bar5;
			uint32_t cardbus_cis_pointer;
			uint16_t subsystem_id;
			uint16_t subsystem_vendor_id;
			uint32_t expansion_rom_bar;
			uint16_t reserved0;
			uint8_t reserved1;
			uint16_t capabilities_ptr;
			uint32_t reserved2;
			uint8_t max_latency;
			uint8_t min_grant;
			uint8_t interrupt_pin;
			uint8_t interrupt_line;
		};
	};
	struct pci_device *next;
} __attribute__((packed));

void register_pci_driver(void);
const struct pci_device *pci_get_device(uint32_t vendor, uint32_t device);
struct pci_device *pci_devices;
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t data);

#endif
