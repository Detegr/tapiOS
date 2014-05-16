#include "devfs.h"
#include <mem/kmalloc.h>
#include <terminal/vga.h>

int register_device(uint16_t major, struct file_actions *fact, struct inode_actions *iact)
{
	if(major == 0) PANIC();

	// Device already registered
	if(devices[major].f_act || devices[major].i_act) return -1;

	devices[major].f_act=fact;
	devices[major].i_act=iact;

	return 0;
}

struct inode *devfs_mknod(struct inode *devfs, const char *name, uint16_t major, uint16_t minor)
{
	// TODO: Add fstype to check if devfs is actually devfs node
	if(!devfs) return NULL;

	struct inode *ret=kmalloc(sizeof(struct inode));
	memset(ret, 0, sizeof(struct inode));
	ret->parent=devfs;
	ret->mountpoint=devfs;
	ret->children=NULL;
	ret->siblings=NULL;

	if(!devfs->children) devfs->children=ret;
	else
	{
		struct inode *i=devfs->children;
		while(i->siblings) i=i->siblings;
		i->siblings=ret;
	}
	strncpy(ret->name, name, 256);
	ret->inode_no=major<<16 | minor;
	ret->size=0;
	ret->f_act=devfs->f_act;
	ret->i_act=devfs->i_act;

	return ret;
}

static int32_t devfs_open(struct file *f, int flags)
{
	uint16_t major=f->inode->inode_no>>16;
	if(major == 0) return 0; // Mountpoint

	f->inode->f_act=devices[major].f_act;
	f->inode->i_act=devices[major].i_act;
	f->inode->f_act->open=devfs_open;

	return 0;
}

static int32_t devfs_stat(struct file *f, struct stat *st)
{
	st->st_dev = -1; // Device id containing file
	st->st_ino = 0;
	st->st_mode = S_IFDIR;
	st->st_mode |= (S_IRWXU | S_IRWXG | S_IRWXO); // All permissions
	st->st_nlink = 0; // Hard links NYI
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_rdev = -1; // Device id (if special file)
	st->st_size = 0;
	st->st_blksize = 0;
	st->st_blocks = 0;
	st->st_atime = 0;
	st->st_ctime = 0;
	st->st_mtime = 0;
	return 0;
}

static struct inode *devfs_search(struct inode *node, const char *name)
{
	return NULL;
}

static struct dirent* devfs_readdir(struct inode *node)
{
	static struct inode *prevnode=NULL;
	struct inode *i=prevnode ? prevnode->siblings : node->children;
	if(i)
	{
		dirent.d_ino=i->inode_no;
		strncpy(dirent.d_name, i->name, 256);
		prevnode=i;
		return &dirent;
	}
	prevnode=NULL;
	return NULL;
}

struct inode *devfs_init(void)
{
	memset(devices, 0, DEVICES_MAX*sizeof(struct device_entry));

	struct inode *ret=kmalloc(sizeof(struct inode));
	memset(ret, 0, sizeof(struct inode));
	ret->inode_no=0;
	ret->size=0;
	ret->f_act=kmalloc(sizeof(struct file_actions));
	ret->i_act=kmalloc(sizeof(struct inode_actions));
	ret->f_act->open=&devfs_open;
	ret->f_act->stat=&devfs_stat;
	ret->i_act->search=&devfs_search;
	ret->i_act->readdir=&devfs_readdir;
	ret->mountpoint=NULL;
	ret->parent=NULL;
	ret->superblock=NULL;
	ret->children=NULL;
	ret->siblings=NULL;

	return ret;
}
