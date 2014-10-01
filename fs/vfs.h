#ifndef _TAPIOS_VFS_H_
#define _TAPIOS_VFS_H_

#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

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

	struct inode *mountpoint;

	struct inode *parent;
	struct inode *children;
	struct inode *siblings;

	void *device;
};

struct file
{
	uint32_t refcount;
	struct inode *inode;
	uint32_t pos;
};

struct file_actions
{
	ssize_t (*read)(struct file *file, void *to, size_t count);
	ssize_t (*write)(struct file *file, const void *data, size_t count);
	int32_t (*stat)(struct file *file, struct stat *st);
	int32_t (*open)(struct file *file, int flags);
	int32_t (*close)(struct file *file);
	int32_t (*ioctl)(struct file *file, int cmd, void *arg);
	int32_t (*poll)(struct file *file, uint16_t events, uint16_t *revents);
};

struct dirent dirent;

struct inode_actions
{
	struct inode *(*new)(struct inode *node, const char *path, int flags);
	struct inode *(*search)(struct inode *node, const char* name);
	struct dirent *(*readdir)(struct inode *node);
};

volatile struct inode *root_fs;

struct inode *vfs_new_inode(struct inode *dir, const char *path, int flags);
struct inode *vfs_search(struct inode *node, const char* name);
struct file *vfs_open(struct inode *node, int *status, int flags);
ssize_t vfs_read(struct file *file, void *to, size_t count);
ssize_t vfs_write(struct file *file, const void *data, size_t count);
int32_t vfs_stat(struct file *file, struct stat *st);
int32_t vfs_close(struct file *f);
int32_t vfs_ioctl(struct file *f, int cmd, void *arg);
int32_t vfs_poll(struct file *f, uint16_t events, uint16_t *revents);
struct dirent *vfs_readdir(int dirfd);
void vfs_mount(struct inode *mount, struct inode *to);

#endif
