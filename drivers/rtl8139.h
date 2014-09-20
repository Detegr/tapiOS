#ifndef RTL8139_TAPIOS_H_
#define RTL8139_TAPIOS_H_

#include <util/util.h>

struct rtl8139
{
	uint8_t bus;
	uint8_t device;
	uint32_t io_base;
	uint8_t irq;
	uint8_t mac[6];

	uint8_t *rx_buf;
	uint16_t rx_pos;

	uint8_t *tx_buf[4];
	uint8_t tx_pos;
	uint8_t tx_buffers_free;
};

void register_rtl8139_driver(void);
void rtl8139_tx(struct rtl8139 *rtl, const uint8_t *data, size_t len);

#endif
