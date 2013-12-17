#include "vfs.h"
#include "../util.h"

static struct inode *walk_path(struct inode *node, const char* name)
{
	struct inode *wnode=node;
	char tokname[256];
	memcpy(tokname, name, 256);
	char* path=strtok(tokname, '/');
	if(path)
	{
		do
		{
			if(!wnode) return NULL;
			wnode=node->actions->search(wnode, path);
		} while((path=strtok(NULL, '/')));
		return wnode;
	}
	return NULL;
}

struct inode *vfs_search(struct inode *node, const char *name)
{
	if(node->actions->search)
	{
		return walk_path(node, name);
	}
	return NULL;
}
