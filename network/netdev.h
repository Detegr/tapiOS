#ifndef _TAPIOS_NETDEV_H_
#define _TAPIOS_NETDEV_H_

#include <util/util.h>
#include <fs/vfs.h>

struct network_device
{
	uint32_t io_base;
	uint8_t irq;
	uint8_t mac[6];

	uint8_t *rx_buf;
	uint16_t rx_pos;

	uint8_t *tx_buf[4];
	uint8_t tx_pos;
	uint8_t tx_buffers_free;

	struct network_actions *n_act;
	struct network_device *next;
};

struct network_actions
{
	ssize_t (*tx)(struct file *file, const void *data, size_t count);
};

struct network_device *network_devices;
struct inode *alloc_netdev_inode(void);
void setup_network(void);
void register_network_device(struct network_device *dev);
void dump_mac_addr(struct network_device *dev);
ssize_t tx_tcp(struct file *file, const void *data, size_t count);

#endif
