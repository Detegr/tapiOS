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

void register_pci_driver(void);

#endif
