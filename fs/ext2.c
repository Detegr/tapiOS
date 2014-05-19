#include "ext2.h"
#include <terminal/vga.h>
#include <util/util.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/kmalloc.h>
#include <stdbool.h>
#include <dirent.h>
#include <limits.h>
#include <sys/poll.h>

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
	kprintf("bitmap_block_no 0x%x\n", b->bitmap_block_no);
	kprintf("bitmap_inode_no 0x%x\n", b->bitmap_inode_no);
	kprintf("inode_table_block %d\n", b->inode_table_block);
	kprintf("unallocated_blocks %d\n", b->unallocated_blocks);
	kprintf("unallocated_inodes %d\n", b->unallocated_inodes);
	kprintf("directory_count %d\n", b->directory_count);
}

static void DEBUG_print_inode(ext2_inode *inode)
{
	kprintf("uid: %d\n", inode->uid);
	kprintf("type and permissions: %x\n", inode->type_and_permissions);
	kprintf("hard link count: %d\n", inode->hard_link_count);
}

static void* get_block(const ext2_superblock* b, int block)
{
	void* p=(void*)b;
	return p - SUPERBLOCK_SIZE + (BLOCKSIZE(b) * block);
}

static inline bool inside_dir(ext2_directory *dir, ext2_directory *dir_base, uint32_t dirsize)
{
	return (dir && ((uint32_t)dir-(uint32_t)dir_base < dirsize));
}

static bool advance(ext2_directory **d)
{
	void* p=*d;
	p+=(*d)->size;
	ext2_directory* ret=p;
	*d = p;
	return ret->inode!=0;
}

static void advance_to_last(ext2_directory **d, int blocksize)
{
	ext2_directory *bdir=*d;
	ext2_directory *d2=*d;
	while(inside_dir(*d, bdir, blocksize))
	{
		d2=*d;
		advance(d);
	}
	*d=d2;
}

static inline ext2_blockgroup_descriptor *get_blockgroup_descriptor_table(const ext2_superblock *sb)
{
	return (ext2_blockgroup_descriptor*)((uint8_t*)sb + SUPERBLOCK_SIZE);
}

static ext2_inode* read_inode(const ext2_superblock* sb, int inode_index)
{
	int blockgroup=(inode_index-1) / sb->inodes_in_blockgroup;
	int index=(inode_index-1) % sb->inodes_in_blockgroup;
	int inode_size=sb->version_major >= 1 ? sb->inode_size : 128;
	int block=(index * inode_size) / BLOCKSIZE(sb);
	ext2_blockgroup_descriptor* bg_table=get_blockgroup_descriptor_table(sb);
	ext2_inode* inode_table=get_block(sb, bg_table[blockgroup].inode_table_block);
	return &inode_table[index];
}

static inline uint32_t min(uint32_t a, uint32_t b)
{
	if(a<b) return a; else return b;
}

static struct dirent* ext2_readdir(struct inode *node)
{
	static ext2_directory *bdir=NULL;
	static ext2_directory *dir=NULL;
	static struct inode *prevnode=NULL;
	ext2_inode *inode=NULL;

	if(prevnode==node)
	{
		if(inside_dir(dir, bdir, BLOCKSIZE(node->superblock)))
		{
			dirent.d_ino=dir->inode;
			memcpy(&dirent.d_name, &dir->name, dir->name_length_low);
			dirent.d_name[dir->name_length_low]=0;
			advance(&dir);
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
		if(inside_dir(bdir, dir, BLOCKSIZE(node->superblock)))
		{
			dirent.d_ino=dir->inode;
			memcpy(&dirent.d_name, &dir->name, dir->name_length_low);
			dirent.d_name[dir->name_length_low]=0;
			advance(&dir);
			return &dirent;
		}
		else return NULL;
	}
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
				struct inode *ret=kmalloc(sizeof(struct inode));
				memset(ret, 0, sizeof(struct inode));
				memcpy(ret->name, &dir->name, dir->name_length_low);
				ret->inode_no=dir->inode;
				ret->flags=retinode->type_and_permissions;
				ret->size=retinode->size_low;
				ret->superblock=node->superblock;
				ret->i_act=node->i_act;
				ret->f_act=node->f_act;
				ret->mountpoint=node->mountpoint;
				ret->children=NULL;
				ret->siblings=NULL;
				return ret;
			}
		}
		advance(&dir);
	}
	return NULL;
}

static uint16_t calculate_dir_size(ext2_directory *d)
{
	if(d->name_length_low > (sizeof(ext2_directory)-8))
	{
		uint16_t ret=d->name_length_low + 8;
		// Align by 4 bytes
		uint16_t off = (ret & 0x3);
		if(off!=0) ret += (4-off);
		return ret;
	}
	return sizeof(ext2_directory);
}

static void do_insert(ext2_superblock *sb, ext2_directory *dir, uint32_t inode_no, const char *name)
{
	advance_to_last(&dir, BLOCKSIZE(sb));
	uint16_t oldsize=dir->size;
	dir->size=calculate_dir_size(dir);
	uint16_t newsize=dir->size;
	advance(&dir);
	int namelen=strnlen(name, 256);
	strncpy((char*)&dir->name, name, namelen+1);
	dir->type_indicator=2;
	dir->name_length_low=namelen;
	dir->inode=inode_no;
	dir->size=oldsize-newsize;
	//kprintf("Successfully inserted\n");
}

static void insert_inode_to_directory(ext2_superblock *sb, ext2_inode *fsdir, uint32_t inode_no, const char *path)
{
	// TODO: Other data blocks
	ext2_directory *dir=get_block(sb, fsdir->blocks[0]);
	do_insert(sb, dir, inode_no, basename((char*)path));
	return;
}

static int get_blockgroup_count(ext2_superblock *sb)
{
	if(!sb) PANIC();
	int c1 = DIV_ROUND_UP(sb->blocks, sb->blocks_in_blockgroup);
	int c2 = DIV_ROUND_UP(sb->inodes, sb->inodes_in_blockgroup);
	if(c1 != c2) PANIC();
	return c1;
}

static int get_free_block(ext2_superblock *sb, uint32_t bitmap_block_no)
{
	uint32_t *block_bitmap=get_block(sb, bitmap_block_no);
	uint32_t len=BLOCKSIZE(sb)/sizeof(uint32_t);
	int block_index=-1;
	for(uint32_t i=0; i<len; ++i)
	{
		if(block_bitmap[i] != 0xFFFFFFFF)
		{// TODO: This still does not work correctly
			//kprintf("%x\n", block_bitmap[i]);
			GET_AND_SET_LSB(block_index, &block_bitmap[i]);
			if(block_index==32) continue;
			//kprintf("%x\n", block_bitmap[i]);
			return block_index;
		}
	}
	return -1;
}

static void write_directory_structure(ext2_superblock *sb, void *block, uint32_t inode_index)
{// Writes ext2_directory entries for '.' and '..' that are expected to found in every directory
	uint8_t *p=block;
	ext2_directory *dp=block;
	dp->type_indicator=-1;
	dp->name_length_low=1;
	dp->name='.';
	uint32_t firstsize=dp->size=calculate_dir_size(block);
	dp->inode=inode_index;

	dp=block+dp->size;
	dp->type_indicator=-1;
	dp->name_length_low=2;
	char* name=(char*)&dp->name;
	name[0]='.'; name[1]='.';
	dp->size=BLOCKSIZE(sb)-firstsize;
	dp->inode=inode_index;
}

struct inode *ext2_new_inode(struct inode *node, const char *path, int flags)
{
	ext2_superblock *sb=node->superblock;
	int blockgroups=get_blockgroup_count(sb);
	for(int blockgroup=0; blockgroup<blockgroups; ++blockgroup)
	{
		ext2_blockgroup_descriptor *desc=&get_blockgroup_descriptor_table(sb)[blockgroup];
		if(desc->unallocated_inodes==0) continue;
		uint32_t *inode_bitmap=get_block(sb, desc->bitmap_inode_no);
		uint32_t len=BLOCKSIZE(sb)/sizeof(uint32_t);
		int inode_index=-1;
		for(uint32_t i=0; i<len; ++i)
		{
			if(inode_bitmap[i] != 0xFFFFFFFF)
			{
				GET_AND_SET_LSB(inode_index, &inode_bitmap[i]);
				desc->unallocated_inodes--;
				break;
			}
		}
		inode_index++; // inode indexes start from 1 but in bitmap they start from 0, so adjust
		if(inode_index<0)
		{
			kprintf("Couldn't find free inode\n");
			PANIC();
		}
		//kprintf("Found free inode: %d\n", inode_index);
		ext2_inode *inode=read_inode(sb, inode_index);
		memset(inode, 0, sizeof(ext2_inode));
		inode->type_and_permissions=flags;
		inode->hard_link_count=1;
		inode->size_low=0;
		inode->size_high=0;
		inode->fragment_size=0;
		insert_inode_to_directory(sb, read_inode(sb, node->inode_no), inode_index, path);

		if(flags & 0x4000)
		{// Directory needs ext2_directory structure to be written to the block
			int block_index=get_free_block(sb, desc->bitmap_block_no);
			if(block_index < 0) return NULL;
			desc->unallocated_blocks--;
			ext2_directory *block=get_block(sb, block_index);
			inode->blocks[0]=block_index;
			write_directory_structure(sb, block, inode_index);
		}

		struct inode *ret=kmalloc(sizeof(struct inode));
		memset(ret, 0, sizeof(struct inode));
		ret->inode_no=inode_index;
		ret->flags=flags;
		strncpy(ret->name, basename((char*)path), 256);
		ret->size=0;
		ret->superblock=sb;
		ret->i_act=node->i_act;
		ret->f_act=node->f_act;
		ret->mountpoint=node->mountpoint;
		return ret;
	}
	kprintf("NYI: Allocate new blockgroup\n");
	PANIC();
	return NULL;
}

static int32_t ext2_open(struct file *f, int flags)
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
		if(i == count) break;
		else if(f && pos >= f->inode->size) break;
		to[pos++]=block[i];
	}
	if(f) f->pos=pos;
	return ret;
}

static int read_indirect_block(ext2_superblock *sb, struct file *f, uint32_t *blocks, uint8_t *to, uint32_t count)
{
	int ret=0;
	uint32_t blockcount = BLOCKSIZE(sb) >> 2; // Blocksize / sizeof(char*)
	for(uint32_t i=0; i<blockcount; ++i)
	{
		ret+=read_block(sb, f, get_block(sb, blocks[i]), to, count-ret);
		if(ret >= count || f->pos >= f->inode->size) break;
	}
	return ret;
}

static int read_blocks(ext2_superblock *sb, struct file *f, uint8_t *to, uint32_t *blocks, uint32_t count)
{
	int ret=0;
	if(f->pos >= f->inode->size) return 0;
	for(int b=0; b<=FINAL_DIRECT_BLOCK; ++b)
	{
		ret+=read_block(sb, f, get_block(sb, blocks[b]), to, count-ret);
		if(ret >= count || f->pos >= f->inode->size) break;
	}
	if(blocks[SINGLY_INDIRECT_BLOCK])
	{
		ret+=read_indirect_block(sb, f, get_block(sb, blocks[SINGLY_INDIRECT_BLOCK]), to, count-ret);
	}
	if(blocks[DOUBLY_INDIRECT_BLOCK])
	{
		uint32_t *doubly_indirect_block=kmalloc(BLOCKSIZE(sb) * sizeof(uint32_t));
		memset(doubly_indirect_block, 0, BLOCKSIZE(sb) * sizeof(uint32_t));
		read_block(sb, NULL, get_block(sb, blocks[DOUBLY_INDIRECT_BLOCK]), (uint8_t*)doubly_indirect_block, BLOCKSIZE(sb));
		for(uint32_t i=0; i<BLOCKSIZE(sb); ++i)
		{
			ret+=read_indirect_block(sb, f, get_block(sb, doubly_indirect_block[i]), to, count-ret);
		}
		kfree(doubly_indirect_block);
	}
	if(blocks[TRIPLY_INDIRECT_BLOCK])
	{
		PANIC();
		/*
		uint8_t doubly_indirect_block[BLOCKSIZE(sb)*sizeof(uint32_t)];
		read_block(sb, NULL, get_block(sb, blocks[DOUBLY_INDIRECT_BLOCK]), doubly_indirect_block, BLOCKSIZE(sb));
		for(uint32_t i=0; i<BLOCKSIZE(sb); ++i)
		{
			uint8_t triply_indirect_block[BLOCKSIZE(sb)*sizeof(uint32_t)];
			read_block(sb, NULL, get_block(sb, doubly_indirect_block[i]), triply_indirect_block, BLOCKSIZE(sb));
			for(uint32_t j=0; j<BLOCKSIZE(sb); ++j)
			{
				ret+=read_indirect_block(sb, f, get_block(sb, triply_indirect_block[j]), to, count);
			}
		}
		*/
	}
	return ret;
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

static int32_t ext2_write(struct file *f, void *data, uint32_t count)
{
	ext2_superblock *sb=f->inode->superblock;
	if(count > BLOCKSIZE(sb)) {kprintf("NYI");PANIC();}
	int blockgroups=get_blockgroup_count(sb);
	for(int blockgroup=0; blockgroup<blockgroups; ++blockgroup)
	{
		ext2_blockgroup_descriptor *desc=&get_blockgroup_descriptor_table(sb)[blockgroup];
		if(desc->unallocated_blocks==0) continue;

		if(f->pos!=0) PANIC(); // NYI
		int block_index=get_free_block(sb, desc->bitmap_block_no);
		if(block_index < 0) return -1;
		desc->unallocated_blocks--;
		ext2_inode *inode=read_inode(sb, f->inode->inode_no);
		uint8_t *block=get_block(sb, block_index);
		uint8_t *from=data;
		int32_t written=0;
		if(count > BLOCKSIZE(sb)) PANIC();
		for(uint32_t i=0; i<count; ++i, ++written)
		{
			block[i]=from[i];
		}
		f->inode->size+=written;
		inode->size_low+=written;
		f->pos+=written;
		inode->blocks[0]=block_index;
		return written;
	}
	return -1;
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

static int32_t ext2_poll(struct file *f, uint16_t events, uint16_t *revents)
{
	if(events & POLLIN)
	{
		if(f->pos < f->inode->size)
		{
			*revents|=POLLIN;
			return 1;
		}
	}
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
	memset(ret, 0, sizeof(struct inode));
	ext2_inode* inode=read_inode(sb, 2); // inode 2 is always the root node
	ext2_directory* d=get_block(sb, inode->blocks[0]);
	memcpy(ret->name, &d->name, d->name_length_low);
	ret->inode_no=2;
	ret->flags=inode->type_and_permissions;
	ret->size=inode->size_low;
	ret->superblock=sb;
	ret->i_act=kmalloc(sizeof(struct inode_actions));
	memset(ret->i_act, 0, sizeof(struct inode_actions));
	ret->f_act=kmalloc(sizeof(struct file_actions));
	memset(ret->f_act, 0, sizeof(struct file_actions));
	ret->i_act->new=&ext2_new_inode;
	ret->i_act->search=&ext2_search;
	ret->i_act->readdir=&ext2_readdir;
	ret->f_act->open=&ext2_open;
	ret->f_act->read=&ext2_read;
	ret->f_act->write=&ext2_write;
	ret->f_act->stat=&ext2_stat;
	ret->f_act->ioctl=NULL;
	ret->f_act->poll=&ext2_poll;
	ret->mountpoint=ret;
	ret->parent=NULL;
	ret->children=NULL;
	ret->siblings=NULL;

	print_startup_info("ext2", true);
	return ret;
}
