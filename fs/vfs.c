#include "vfs.h"
#include <mem/kmalloc.h>
#include <util/util.h>
#include <task/process.h>
#include <terminal/vga.h>

static inline int names_eq(const char *n1, const char *n2)
{
	return memcmp(n1, n2, max(strnlen(n1, 256), strnlen(n2, 256)));
}

static struct inode *walk_path_do(struct inode *node, const char *name, struct inode *(walkfunc)(struct inode *, const char *))
{
	if(strlen(name) == 1 && name[0] == '/') return (struct inode*)root_fs;
	struct inode *wnode=node;
	char tokname[256];
	memcpy(tokname, name, 256);
	char* path=strtok(tokname, '/');
	if(path)
	{
		do wnode=walkfunc(wnode, path); while((path=strtok(NULL, '/')));
		return wnode;
	}
	return NULL;
}

static struct inode *searchfunc(struct inode *wnode, const char *path)
{
	if(!wnode) return NULL;
	if(wnode->children)
	{
		struct inode *i=wnode->children;
		struct inode *exists=NULL;
		do
		{
			if(names_eq(path, i->name) == 0)
			{
				//kprintf("Found %s from cache\n", path);
				exists=i;
				break;
			}
		} while((i=i->siblings));
		if(exists) return exists;
	}
	if(!wnode->i_act || !wnode->i_act->search) return NULL;
	struct inode *nextnode=wnode->i_act->search(wnode, path);
	if(nextnode)
	{
		if(!wnode->children) wnode->children=nextnode;
		else
		{
			struct inode *i=wnode->children;
			while(i->siblings) i=i->siblings;
			i->siblings=nextnode;
		}
		nextnode->parent=wnode;
	}
	return nextnode;
}

struct inode *vfs_search(struct inode *node, const char *name)
{
	if(node->i_act->search)
	{
		return walk_path_do(node, name, searchfunc);
	}
	return NULL;
}

struct inode *vfs_new_inode(struct inode *dir, const char *path)
{
	if(dir->i_act->new)
	{
		struct inode *ret=dir->i_act->new(dir, path);
		ret->parent=dir;
		ret->siblings=NULL;
		ret->children=NULL;
		struct inode *i=ret->parent->children;
		if(i)
		{
			while(i->siblings) i=i->siblings;
			i->siblings=ret;
		} else ret->parent->children=ret;
		return ret;
	}
	return NULL;
}

struct file *vfs_open(struct inode *node, int *status, int flags)
{
	struct file *ret=kmalloc(sizeof(struct file)); // Will be freed on close
	ret->refcount=1;
	ret->inode=node;
	ret->pos=0;
	int retval;
	if(node->f_act->open)
	{
		if((retval=node->f_act->open(ret, flags)) < 0)
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

int32_t vfs_write(struct file *file, void *data, uint32_t count)
{
	if(file->inode->f_act->write)
	{
		return file->inode->f_act->write(file, data, count);
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
		struct inode *i=f->inode->parent->children;
		while(i)
		{
			if(i->siblings==f->inode)
			{
				i->siblings=f->inode->siblings;
				kfree(f->inode);
				f->inode=NULL;
				break;
			}
			i=i->siblings;
		}
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

void vfs_mount(struct inode *mount, struct inode *to)
{
	if(to->children || !to->parent) PANIC();
	mount->siblings=to->siblings;
	mount->parent=to->parent;
	mount->mountpoint=to->mountpoint;
	strncpy(mount->name, to->name, 256);

	// Add to inode tree
	struct inode *i=to->parent->children;
	if(i == to) to->parent->children=mount;
	else
	{
		while(i->siblings != to) i=i->siblings;
		i->siblings=mount;
	}
}

int32_t vfs_ioctl(struct file *f, int cmd, void *arg)
{
	if(f->inode->f_act->ioctl)
	{
		return f->inode->f_act->ioctl(f, cmd, arg);
	}
	return -EBADF;
}

int32_t vfs_poll(struct file *f, uint16_t events, uint16_t *revents)
{
	if(f->inode->f_act->poll)
	{
		return f->inode->f_act->poll(f, events, revents);
	}
	return 0;
}
