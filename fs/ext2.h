#ifndef _TAPIOS_EXT2_H_
#define _TAPIOS_EXT2_H_

#include <stdint.h>
#include "../vfs.h"

typedef struct
{
	uint32_t inodes;
	uint32_t blocks;
	uint32_t blocks_superuser;
	uint32_t unallocated_blocks;
	uint32_t unallocated_inodes;
	uint32_t superblock_blocknum;
	uint32_t block_size_shift;
	uint32_t fragment_size_shift;
	uint32_t blocks_in_blockgroup;
	uint32_t fragments_in_blockgroup;
	uint32_t inodes_in_blockgroup;
	uint32_t last_mount_time;
	uint32_t last_write_time;
	uint16_t mounted_since_last_check;
	uint16_t mounts_allowed_until_check;
	uint16_t ext2_signature; // Should be 0xef53 if the fs is ext2
	uint16_t fs_state;
	uint16_t error_action;
	uint16_t version_minor;
	uint32_t last_check_time;
	uint32_t force_check_interval_time;
	uint32_t os_id;
	uint32_t version_major;
	uint16_t reserved_blocks_uid;
	uint16_t reserved_blocks_gid;
	// If version_major > 1
	uint32_t first_free_inode;
	uint16_t inode_size;
	uint16_t superblock_block_group;
	uint32_t optional_features;
	uint32_t required_features;
	uint32_t features_requiring_readonly;
	int8_t fs_id[16];
	int8_t volume_name[16];
	int8_t volume_last_mount_path[64];
	uint32_t compression;
	uint8_t preallocate_blocks_for_files;
	uint8_t preallocate_blocks_for_dirs;
	uint16_t unused;
	int8_t journal_id[16];
	uint32_t journal_inode;
	uint32_t journal_device;
	uint32_t orphan_inode_list_head;
} ext2_superblock;

typedef struct
{
	uint32_t bitmap_block_addr;
	uint32_t bitmap_inode_addr;
	uint32_t inode_table_block;
	uint16_t unallocated_blocks;
	uint16_t unallocated_inodes;
	uint16_t directory_count;
	uint8_t unused[14];
} ext2_blockgroup_descriptor;

typedef struct
{
	uint16_t type_and_permissions;
	uint16_t uid;
	uint32_t size_low;
	uint32_t access_time;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t deletion_time;
	uint16_t gid;
	uint16_t hard_link_count;
	uint32_t disk_sector_count;
	uint32_t flags;
	uint32_t os_value;
	uint32_t block0;
	uint32_t block1;
	uint32_t block2;
	uint32_t block3;
	uint32_t block4;
	uint32_t block5;
	uint32_t block6;
	uint32_t block7;
	uint32_t block8;
	uint32_t block9;
	uint32_t block10;
	uint32_t block11;
	uint32_t singly_indirect_block;
	uint32_t doubly_indirect_block;
	uint32_t triply_indirect_block;
	uint32_t generation_number;
	uint32_t file_acl;
	union {
		uint32_t directory_acl;
		uint32_t size_high;
	};
	uint32_t fragment_block_addr;
	uint8_t fragment_number;
	uint8_t fragment_size;
	uint16_t reserved;
	uint64_t tapios_not_used; // Linux uses to store high 16bits of 32bit user id's
} ext2_inode;

typedef struct
{
	uint32_t inode;
	uint16_t size;
	uint8_t name_length_low;
	union {
		uint8_t type_indicator;
		uint8_t name_length_high;
	};
	int8_t name;
} ext2_directory;

fs_node* ext2_fs_init(uint8_t* fs_data);

#endif
