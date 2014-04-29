#ifndef _TAPIOS_DEVFS_H_
#define _TAPIOS_DEVFS_H_

#include "vfs.h"

#define TERMINAL 1
#define KEYBOARD 2

#define DEVICES_MAX 128

struct device_entry
{
	struct file_actions  *f_act;
	struct inode_actions *i_act;
};

struct device_entry devices[DEVICES_MAX];

int register_device(uint16_t major, struct file_actions *fact, struct inode_actions *iact);
struct inode *devfs_mknod(struct inode *devfs, const char *name, uint16_t major, uint16_t minor);
struct inode *devfs_init(void);

#endif
