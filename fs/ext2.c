#include "ext2.h"
#include <terminal/vga.h>
#include <util/util.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/kmalloc.h>
#include <stdbool.h>
#include <dirent.h>

#define SUPERBLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024
#define ISDIR(x) (x->type_and_permissions & 0x4000)

#define BLOCKSIZE(x) (uint32_t)(1024 << ((ext2_superblock*)x)->block_size_shift)

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
	return p - SUPERBLOCK_SIZE + (BLOCKSIZE(b) * block);
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
	int block=(index * inode_size) / BLOCKSIZE(b);
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

static struct dirent* ext2_readdir(struct inode *node)
{
	static ext2_directory *bdir=NULL;
	static ext2_directory *dir=NULL;
	static struct inode *prevnode=NULL;
	ext2_inode *inode=NULL;

	if(prevnode==node)
	{
		if(dir && ((uint32_t)dir-(uint32_t)bdir < BLOCKSIZE(node->superblock)))
		{
			dirent.d_ino=dir->inode;
			memcpy(&dirent.d_name, &dir->name, dir->name_length_low);
			dirent.d_name[dir->name_length_low]=0;
			dir=advance(dir);
			return &dirent;
		}
		else
		{
			prevnode=NULL;
			dir=bdir=NULL;
			return NULL;
		}
	}
	else
	{
		prevnode=node;
		inode=read_inode(node->superblock, node->inode_no);
		if(!(ISDIR(inode)))
		{
			kprintf("Inode is not a directory\n");
			return NULL;
		}
		// TODO: Other data blocks
		bdir=dir=get_block(node->superblock, inode->blocks[0]);
		if(dir && ((uint32_t)bdir-(uint32_t)dir < BLOCKSIZE(node->superblock)))
		{
			dirent.d_ino=dir->inode;
			memcpy(&dirent.d_name, &dir->name, dir->name_length_low);
			dirent.d_name[dir->name_length_low]=0;
			dir=advance(dir);
			return &dirent;
		}
		else return NULL;
	}
}

static inline bool inside_dir(ext2_directory *dir, ext2_directory *dir_base, uint32_t dirsize)
{
	return (dir && ((uint32_t)dir-(uint32_t)dir_base < dirsize));
}

static inline uint32_t min(uint32_t a, uint32_t b)
{
	if(a<b) return a; else return b;
}

struct inode *ext2_search(struct inode *node, const char *name)
{
	ext2_inode *inode=read_inode(node->superblock, node->inode_no);
	if(!(ISDIR(inode))) return NULL; // Searching non-directories makes no sense
	// TODO: Other data blocks
	ext2_directory *bdir;
	ext2_directory *dir=bdir=get_block(node->superblock, inode->blocks[0]);
	while(inside_dir(dir, bdir, BLOCKSIZE(node->superblock)))
	{
		if(strlen(name) == dir->name_length_low)
		{
			if(memcmp(&dir->name, name, dir->name_length_low) == 0)
			{
				ext2_inode *retinode=read_inode(node->superblock, dir->inode);
				struct inode *ret=kmalloc(sizeof(struct inode)); // TODO: Free
				memcpy(ret->name, &dir->name, dir->name_length_low);
				ret->inode_no=dir->inode;
				ret->flags=retinode->type_and_permissions;
				ret->size=retinode->size_low;
				ret->superblock=node->superblock;
				ret->i_act=node->i_act;
				ret->f_act=node->f_act;
				return ret;
			}
		}
		dir=advance(dir);
	}
	return NULL;
}

static int32_t ext2_open(struct file *f)
{
	ext2_inode *inode=read_inode(f->inode->superblock, f->inode->inode_no);
	if(inode) return 0;
	else return -ENOENT;
}

static int read_block(ext2_superblock *sb, struct file *f, uint8_t *block, uint8_t *to, uint32_t count)
{
	/* f is an optional parameter because we don't want to modify the position of
	 * struct file *f when reading doubly or triply indirect blocks.
	 */
	uint32_t pos=f?f->pos:0;
	uint32_t ret=0;
	uint32_t blocksize = BLOCKSIZE(sb);
	for(uint32_t i=0; i<blocksize; ++i, ++ret)
	{
		if(pos == count) break;
		else if(f && pos >= f->inode->size)
		{
			ret=0;
			break;
		}
		to[pos++]=block[i];
	}
	if(f) f->pos=pos;
	return ret;
}

static int read_indirect_block(ext2_superblock *sb, struct file *f, uint32_t *blocks, uint8_t *to, uint32_t count)
{
	uint32_t ret=0;
	uint32_t blockcount = BLOCKSIZE(sb) >> 2; // Blocksize / sizeof(char*)
	for(uint32_t i=0; i<blockcount; ++i)
	{
		ret += read_block(sb, f, get_block(sb, blocks[i]), to, count);
	}
	return ret;
}

static int read_blocks(ext2_superblock *sb, struct file *f, uint8_t *to, uint32_t *blocks, uint32_t count)
{
	if(f->pos >= f->inode->size) return 0;
	for(int b=0; b<=FINAL_DIRECT_BLOCK; ++b)
	{
		read_block(sb, f, get_block(sb, blocks[b]), to, count);
	}
	if(blocks[SINGLY_INDIRECT_BLOCK])
	{
		read_indirect_block(sb, f, get_block(sb, blocks[SINGLY_INDIRECT_BLOCK]), to, count);
	}
	if(blocks[DOUBLY_INDIRECT_BLOCK])
	{
		uint32_t *doubly_indirect_block=kmalloc(BLOCKSIZE(sb) * sizeof(uint32_t));
		memset(doubly_indirect_block, 0, BLOCKSIZE(sb) * sizeof(uint32_t));
		read_block(sb, NULL, get_block(sb, blocks[DOUBLY_INDIRECT_BLOCK]), (uint8_t*)doubly_indirect_block, BLOCKSIZE(sb));
		for(uint32_t i=0; i<BLOCKSIZE(sb); ++i)
		{
			read_indirect_block(sb, f, get_block(sb, doubly_indirect_block[i]), to, count);
		}
		kfree(doubly_indirect_block);
	}
	if(blocks[TRIPLY_INDIRECT_BLOCK])
	{
		uint8_t doubly_indirect_block[BLOCKSIZE(sb)*sizeof(uint32_t)];
		read_block(sb, NULL, get_block(sb, blocks[DOUBLY_INDIRECT_BLOCK]), doubly_indirect_block, BLOCKSIZE(sb));
		for(uint32_t i=0; i<BLOCKSIZE(sb); ++i)
		{
			uint8_t triply_indirect_block[BLOCKSIZE(sb)*sizeof(uint32_t)];
			read_block(sb, NULL, get_block(sb, doubly_indirect_block[i]), triply_indirect_block, BLOCKSIZE(sb));
			for(uint32_t j=0; j<BLOCKSIZE(sb); ++j)
			{
				read_indirect_block(sb, f, get_block(sb, triply_indirect_block[j]), to, count);
			}
		}
	}
	return f->pos;
}

static int32_t ext2_read(struct file *f, void *to, uint32_t count)
{
	ext2_inode *inode=read_inode(f->inode->superblock, f->inode->inode_no);
	if(ISDIR(inode))
	{
		return -EISDIR;
	}
	return read_blocks(f->inode->superblock, f, to, inode->blocks, count);
}

static int32_t ext2_stat(struct file *f, struct stat *st)
{
	ext2_inode *inode=read_inode(f->inode->superblock, f->inode->inode_no);
	st->st_dev = -1; // Device id containing file
	st->st_ino = f->inode->inode_no;
	st->st_mode = ISDIR(inode) ? S_IFDIR : S_IFREG;
	st->st_mode |= (S_IRWXU | S_IRWXG | S_IRWXO); // All permissions
	st->st_nlink = 0; // Hard links NYI
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_rdev = -1; // Device id (if special file)
	st->st_size = f->inode->size;
	st->st_blksize = BLOCKSIZE(f->inode->superblock);
	st->st_blocks = f->inode->size / 512; // TODO: What is this for?
	st->st_atime = inode->access_time;
	st->st_ctime = inode->creation_time;
	st->st_mtime = inode->modification_time;
	return 0;
}

struct inode *ext2_fs_init(uint8_t *fs_data)
{
	ext2_superblock* sb=(ext2_superblock*)(fs_data+SUPERBLOCK_OFFSET);
	if(sb->ext2_signature != 0xef53)
	{
		print_startup_info("ext2", false);
		return NULL;
	}
	struct inode *ret=kmalloc(sizeof(struct inode));
	//int blockgroup_count=((sb->blocks+(sb->blocks_in_blockgroup-1))/sb->blocks_in_blockgroup);
	ext2_inode* inode=read_inode(sb, 2); // inode 2 is always the root node
	ext2_directory* d=get_block(sb, inode->blocks[0]);
	memcpy(ret->name, &d->name, d->name_length_low);
	ret->inode_no=2;
	ret->flags=inode->type_and_permissions;
	ret->size=inode->size_low;
	ret->superblock=sb;
	ret->i_act=kmalloc(sizeof(struct inode_actions));
	ret->f_act=kmalloc(sizeof(struct file_actions));
	ret->i_act->search=&ext2_search;
	ret->i_act->readdir=&ext2_readdir;
	ret->f_act->open=&ext2_open;
	ret->f_act->read=&ext2_read;
	ret->f_act->stat=&ext2_stat;
	print_startup_info("ext2", true);
	return ret;
}
