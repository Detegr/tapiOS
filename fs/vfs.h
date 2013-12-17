#ifndef _TAPIOS_VFS_H_
#define _TAPIOS_VFS_H_

#include <stdint.h>

struct fs_node;

typedef struct fs_actions
{
	uint32_t (*fs_read)(struct fs_node* node, uint32_t size, uint8_t* from);
	uint32_t (*fs_write)(struct fs_node* node, uint32_t size, uint8_t* to);
	void (*fs_open)(struct fs_node* node);
	void (*fs_close)(struct fs_node* node);
	struct dirent* (*fs_readdir)(struct fs_node* node);
	//struct fs_node* (*fs_opendir)(struct fs_node* node, const char* name);
} fs_actions;

typedef struct fs_node
{
	char name[256];
	uint32_t inode;
	uint32_t flags;
	uint32_t length;
	struct fs_node* link;
	void* superblock;
	fs_actions actions;
} fs_node;

struct dirent
{
	uint32_t inode;
	char name[256];
};

struct dirent dirent;
static fs_node root_fs;

uint32_t fs_read(fs_node* node, uint32_t size, uint8_t* from);
struct dirent* fs_readdir(fs_node* node);

#endif
