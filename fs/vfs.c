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

int32_t vfs_open(struct inode *node, struct file *ret)
{
	int32_t retval=0;
	ret->inode=node;
	ret->pos=0;
	if(node->f_act->open)
	{
		retval=node->f_act->open(ret);
		if(retval==-1) return -1;
	}
	else return -1;
	if(!current_process) return retval;

	/*
	struct open_files *of=current_process->files_open;
	if(!of)
	{
		of=current_process->files_open=kmalloc(sizeof(struct open_files));
		of->file=ret;
		of->next=NULL;
	}
	else
	{
		struct open_files *of=current_process->files_open;
		while(of->next) of=of->next;
		of->next=kmalloc(sizeof(struct open_files));
		of->next->next=NULL;
		of=of->next;
		of->file=ret;
		of->next=NULL;
	}
	*/
	return retval;
}

int32_t vfs_read(struct file *file, void *to, uint32_t count)
{
	if(file->inode->f_act->read)
	{
		return file->inode->f_act->read(file, to, count);
	}
	return -1;
}

struct dirent *vfs_readdir(DIR *dirp)
{
	struct file *filep=current_process->fds[dirp->dir_fd];
	struct inode *inode=filep->inode;
	if(!inode || !inode->i_act || !inode->i_act->readdir) return NULL;
	return inode->i_act->readdir(inode);
}
