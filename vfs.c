#include "vfs.h"
#include "util.h"

uint32_t fs_read(fs_node* node, uint32_t size, uint8_t* from)
{
	if(node->actions.fs_read)
	{
		return node->actions.fs_read(node, size, from);
	}
	return 0;
}

struct dirent* fs_readdir(fs_node* node)
{
	if(node->actions.fs_readdir)
	{
		return node->actions.fs_readdir(node);
	}
	return NULL;
}
