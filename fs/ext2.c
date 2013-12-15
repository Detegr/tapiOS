#include "ext2.h"
#include "../vga.h"
#include "../util.h"
#include "../pmm.h"
#include "../vmm.h"
#include "../heap.h"

#define SUPERBLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024

static void DEBUG_print_superblock(ext2_superblock* b)
{
	kprintf("inodes: %d\n", b->inodes);
	kprintf("blocks: %d\n", b->blocks);
	kprintf("blocks_superuser: %d\n", b->blocks_superuser);
	kprintf("unallocated_blocks: %d\n", b->unallocated_blocks);
	kprintf("unallocated_inodes: %d\n", b->unallocated_inodes);
	kprintf("superblock_blocknum: %d\n", b->superblock_blocknum);
	kprintf("block_size_shift: %d\n", b->block_size_shift);
	kprintf("fragment_size_shift: %d\n", b->fragment_size_shift);
	kprintf("blocks_in_blockgroup: %d\n", b->blocks_in_blockgroup);
	kprintf("fragments_in_blockgroup: %d\n", b->fragments_in_blockgroup);
	kprintf("inodes_in_blockgroup: %d\n", b->inodes_in_blockgroup);
	kprintf("last_mount_time: %d\n", b->last_mount_time);
	kprintf("last_write_time: %d\n", b->last_write_time);
	kprintf("mounted_since_last_check: %d\n", b->mounted_since_last_check);
	kprintf("mounts_allowed_until_check: %d\n", b->mounts_allowed_until_check);
	kprintf("ext2_signature: 0x%x\n", b->ext2_signature); // Should be 0xef53 if the fs is ext2
	kprintf("fs_state: %d\n", b->fs_state);
	kprintf("error_action: %d\n", b->error_action);
	kprintf("version_minor: %d\n", b->version_minor);
	kprintf("last_check_time: %d\n", b->last_check_time);
	kprintf("force_check_interval_time: %d\n", b->force_check_interval_time);
	kprintf("os_id: %d\n", b->os_id);
	kprintf("version_major: %d\n", b->version_major);
	kprintf("reserved_blocks_uid: %d\n", b->reserved_blocks_uid);
	kprintf("reserved_blocks_gid: %d\n", b->reserved_blocks_gid);
}

static void DEBUG_print_blockgroup_descriptor(ext2_blockgroup_descriptor* b)
{
	kprintf("bitmap_block_addr 0x%x\n", b->bitmap_block_addr);
	kprintf("bitmap_inode_addr 0x%x\n", b->bitmap_inode_addr);
	kprintf("inode_table_block %d\n", b->inode_table_block);
	kprintf("unallocated_blocks %d\n", b->unallocated_blocks);
	kprintf("unallocated_inodes %d\n", b->unallocated_inodes);
	kprintf("directory_count %d\n", b->directory_count);
}

static void* get_block(ext2_superblock* b, int block)
{
	void* p=b;
	return p - SUPERBLOCK_SIZE + ((1024 << b->block_size_shift) * block);
}

static ext2_directory* advance(ext2_directory* d)
{
	void* p=d;
	p+=d->size;
	ext2_directory* ret=p;
	if(ret->inode==0) return NULL;
	else return ret;
}

static ext2_inode* read_inode(ext2_superblock* b, int inode_index)
{
	int blockgroup=(inode_index-1) / b->inodes_in_blockgroup;
	int index=(inode_index-1) % b->inodes_in_blockgroup;
	int inode_size=b->version_major >= 1 ? b->inode_size : 128;
	int block=(index * inode_size) / (1024 << b->block_size_shift);
	ext2_blockgroup_descriptor* bg_table=(ext2_blockgroup_descriptor*)((uint8_t*)b + SUPERBLOCK_SIZE + (blockgroup * sizeof(ext2_blockgroup_descriptor)));
	ext2_blockgroup_descriptor* bg=&bg_table[blockgroup];
	ext2_inode* inode_table=get_block(b, bg->inode_table_block);
	return &inode_table[index];
	/*
	if(inode->type_and_permissions & 0x8000)
	{
		kprintf("Contents of inode %d:\n", inode_index);
		uint8_t* p=get_block(b, inode->block0);
		for(uint32_t i=0; i<inode->size_low; ++i)
		{
			kprintf("%c", p[i]);
		}
	}
	return NULL;*/
}

static struct dirent* ext2_readdir(fs_node* node)
{
	static ext2_inode* inode=NULL;
	static ext2_directory* bdir=NULL;
	static ext2_directory* dir=NULL;
	static fs_node* prevnode=NULL;

	if(prevnode==node)
	{
		if(dir && ((uint32_t)dir-(uint32_t)bdir < 1024))
		{
			dirent.inode=dir->inode;
			memcpy(&dirent.name, &dir->name, dir->name_length_low);
			dirent.name[dir->name_length_low]=0;
			dir=advance(dir);
			return &dirent;
		}
		else return NULL;
	}
	else
	{
		prevnode=node;
		inode=read_inode(node->superblock, node->inode);
		if(!(inode->type_and_permissions & 0x4000))
		{
			kprintf("Inode is not a directory\n");
			return NULL;
		}
		// TODO: Other data blocks
		bdir=dir=get_block(node->superblock, inode->block0);
		if(dir && ((uint32_t)bdir-(uint32_t)dir < 1024))
		{
			dirent.inode=dir->inode;
			memcpy(&dirent.name, &dir->name, dir->name_length_low);
			dirent.name[dir->name_length_low]=0;
			dir=advance(dir);
			return &dirent;
		}
		else return NULL;
	}
}

fs_node* ext2_fs_init(uint8_t* fs_data)
{
	ext2_superblock* sb=(ext2_superblock*)(fs_data+SUPERBLOCK_OFFSET);
	if(sb->ext2_signature != 0xef53)
	{
		print_startup_info("ext2", false);
		return NULL;
	}
	fs_node* ret=kmalloc(sizeof(fs_node));
	//int blockgroup_count=((sb->blocks+(sb->blocks_in_blockgroup-1))/sb->blocks_in_blockgroup);
	ext2_inode* inode=read_inode(sb, 2); // inode 2 is always the root node
	ext2_directory* d=get_block(sb, inode->block0);
	memcpy(ret->name, &d->name, d->name_length_low);
	ret->inode=2;
	ret->flags=inode->type_and_permissions;
	ret->length=inode->size_low;
	ret->superblock=sb;
	ret->link=NULL;
	ret->actions.fs_readdir=&ext2_readdir;
	print_startup_info("ext2", true);
	return ret;
}
