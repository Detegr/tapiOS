#include "vfs.h"
#include <mem/kmalloc.h>
#include <util/util.h>
#include <task/process.h>
#include <terminal/vga.h>

static struct inode *walk_path(struct inode *node, const char* name)
{
	if(strlen(name) == 1 && name[0] == '/') return (struct inode*)root_fs;

	struct inode *wnode=node;
	char tokname[256];
	memcpy(tokname, name, 256);
	char* path=strtok(tokname, '/');
	if(path)
	{
		do
		{
			if(!wnode) return NULL;
			struct inode *nextnode=node->i_act->search(wnode, path);
			if(wnode!=root_fs) kfree(wnode);
			wnode = nextnode;
		} while((path=strtok(NULL, '/')));
		return wnode;
	}
	return NULL;
}

struct inode *vfs_search(struct inode *node, const char *name)
{
	if(node->i_act->search)
	{
		return walk_path(node, name);
	}
	return NULL;
}

struct file *vfs_open(struct inode *node, int *status)
{
	struct file *ret=kmalloc(sizeof(struct file)); // Will be freed on close
	ret->refcount=1;
	ret->inode=node;
	ret->pos=0;
	int retval;
	if(node->f_act->open)
	{
		if((retval=node->f_act->open(ret)) < 0)
		{
			kfree(ret);
			if(status) *status=-retval;
			return NULL;
		}
	}
	else
	{
		kfree(ret);
		if(status) *status=-EBADF;
		return NULL;
	}
	if(!current_process) PANIC();
	return ret;
}

int32_t vfs_read(struct file *file, void *to, uint32_t count)
{
	if(file->inode->f_act->read)
	{
		return file->inode->f_act->read(file, to, count);
	}
	return -EBADF;
}

int32_t vfs_close(struct file *f)
{
	if(!f->inode) return -1;
	if(f->refcount == 0) PANIC();
	if(--f->refcount == 0 && f->inode)
	{
		if(f->inode == root_fs) return 0;
		kfree(f->inode);
	}
	return 0;
}

struct dirent *vfs_readdir(int dirfd)
{
	struct file *filep=current_process->fds[dirfd];
	struct inode *inode=filep->inode;
	if(!inode || !inode->i_act || !inode->i_act->readdir) return NULL;
	return inode->i_act->readdir(inode);
}

int32_t vfs_stat(struct file *file, struct stat *st)
{
	if(file->inode->f_act->stat)
	{
		return file->inode->f_act->stat(file, st);
	}
	return -EBADF;
}
