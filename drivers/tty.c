#include "tty.h"
#include <fs/devfs.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <util/scancodes.h>
#include <task/process.h>

struct file_actions tty_fact;
struct inode_actions tty_iact;

int32_t tty_write(struct file *f, void *to, uint32_t count)
{
	char* str=(char*)to;
	for(uint32_t i=0; i<count; ++i)
	{
		kprintc(str[i]);
	}
	return count;
}

void register_tty_driver(void)
{
	tty_fact.write=&tty_write;

	register_device(TERMINAL, &tty_fact, &tty_iact);
	struct inode *devfs=vfs_search((struct inode*)root_fs, "/dev");
	devfs_mknod(devfs, "terminal", TERMINAL, 0);
}
