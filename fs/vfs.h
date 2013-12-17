#ifndef _TAPIOS_VFS_H_
#define _TAPIOS_VFS_H_

#include <stdint.h>

struct inode_actions;

struct inode
{
	uint32_t inode_no;
	uint32_t flags;
	uint32_t size;
	char name[256];
	void *superblock;
	struct inode_actions *actions;
};

struct file_actions
{
	uint32_t (*read)(struct inode *node, uint32_t size, uint8_t *from);
	uint32_t (*write)(struct inode *node, uint32_t size, uint8_t *to);
	uint32_t (*open)(struct inode *node);
	uint32_t (*close)(struct inode *node);
	uint32_t (*readdir)(struct inode *node, void* to);
};

struct inode_actions
{
	struct inode *(*search)(struct inode *node, const char* name);
};

struct dirent
{
	uint32_t inode;
	char name[256];
};
struct dirent dirent;

static struct inode root_fs;
struct inode *vfs_search(struct inode *node, const char* name);

#endif
