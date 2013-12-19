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
	struct inode_actions *i_act;
	struct file_actions *f_act;
};

struct file
{
	struct inode *inode;
	uint32_t pos;
};

struct file_actions
{
	int32_t (*read)(struct file *file, void *to, uint32_t count);
	int32_t (*write)(struct file *file, void *from, uint32_t count);
	int32_t (*open)(struct file *file);
	int32_t (*close)(struct file *file);
};

struct dirent
{
	uint32_t inode;
	char name[256];
};
struct dirent dirent;

struct inode_actions
{
	struct inode *(*search)(struct inode *node, const char* name);
	struct dirent *(*readdir)(struct inode *node);
};

struct open_files
{
	struct file *file;
	struct open_files *next;
};

volatile struct inode *root_fs;

struct inode *vfs_search(struct inode *node, const char* name);
int32_t vfs_open(struct inode *node, struct file *file);
int32_t vfs_read(struct file *file, void *to, uint32_t count);

#endif
