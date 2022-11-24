/*
 * Copyright 2022 Luis Henrique O. Rios
 *
 * This file is part of MyOS kernel.
 *
 * MyOS kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MyOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyOS. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KERNEL_EXT2_FILE_SYSTEM_H
	#define KERNEL_EXT2_FILE_SYSTEM_H

	#include <stdint.h>

	#include <sys/stat.h>

	#include "kernel/cmos.h"
	#include "kernel/memory_manager.h"

	#include "kernel/file_system/file_system.h"

	#include "kernel/io/block_device.h"

	#include "util/b_tree.h"

	/*
	 * References:
	 * - The Second Extended File System - Internal Layout
	 */
	#define EXT2_SUPERBLOCK_OFFSET 1024

	#define EXT2_GOOD_OLD_REV 0
	#define EXT2_DYNAMIC_REV 1

	#define EXT2_BAD_INO 1
	#define EXT2_ROOT_INO 2
	#define EXT2_ACL_IDX_INO 3
	#define EXT2_ACL_DATA_INO 4
	#define EXT2_BOOT_LOADER_INO 5
	#define EXT2_UNDEL_DIR_INO 6

	#define EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT 12
	#define EXT2_SINGLE_INDIRECTION_ENTRY_INDEX 12
	#define EXT2_DOUBLE_INDIRECTION_ENTRY_INDEX 13
	#define EXT2_TRIPLE_INDIRECTION_ENTRY_INDEX 14

	#define EXT2_SYMBOLIC_LINK_SELF_CONTAINED_DATA_MAX_SIZE 60

	#define EXT2_FT_UNKNOWN 0
	#define EXT2_FT_REG_FILE 1
	#define EXT2_FT_DIR 2
	#define EXT2_FT_CHRDEV 3
	#define EXT2_FT_BLKDEV 4
	#define EXT2_FT_FIFO 5
	#define EXT2_FT_SOCK 6
	#define EXT2_FT_SYMLINK 7

	#define EXT2_S_IFSOCK 0xC000
	_Static_assert(EXT2_S_IFSOCK == S_IFSOCK, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_IFLNK 0xA000
	_Static_assert(EXT2_S_IFLNK == S_IFLNK, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_IFREG 0x8000
	_Static_assert(EXT2_S_IFREG == S_IFREG, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_IFBLK 0x6000
	_Static_assert(EXT2_S_IFBLK == S_IFBLK, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_IFDIR 0x4000
	_Static_assert(EXT2_S_IFDIR == S_IFDIR, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_IFCHR 0x2000
	_Static_assert(EXT2_S_IFCHR == S_IFCHR, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_IFIFO 0x1000
	_Static_assert(EXT2_S_IFIFO == S_IFIFO, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_S_ISUID 0x0800
	_Static_assert(EXT2_S_ISUID == S_ISUID, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_ISGID 0x0400
	_Static_assert(EXT2_S_ISGID == S_ISGID, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_ISVTX 0x0200
	_Static_assert(EXT2_S_ISVTX == S_ISVTX, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	/* Owner: */
	#define EXT2_S_IRUSR 0x0100
	_Static_assert(EXT2_S_IRUSR == S_IRUSR, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_IWUSR 0x0080
	_Static_assert(EXT2_S_IWUSR == S_IWUSR, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_IXUSR 0x0040
	_Static_assert(EXT2_S_IXUSR == S_IXUSR, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	/* Group: */
	#define EXT2_S_IRGRP 0x0020
	_Static_assert(EXT2_S_IRGRP == S_IRGRP, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_IWGRP 0x0010
	_Static_assert(EXT2_S_IWGRP == S_IWGRP, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_IXGRP 0x0008
	_Static_assert(EXT2_S_IXGRP == S_IXGRP, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	/* Other users: */
	#define EXT2_S_IROTH 0x0004
	_Static_assert(EXT2_S_IROTH == S_IROTH, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_IWOTH 0x0002
	_Static_assert(EXT2_S_IWOTH == S_IWOTH, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");
	#define EXT2_S_IXOTH 0x0001
	_Static_assert(EXT2_S_IXOTH == S_IXOTH, "The Ext2 constant should match the respective value declared on \"<sys/stat.h>\" header.");

	#define EXT2_SUPER_MAGIC 0xEF53

	#define EXT2_FEATURE_INCOMPAT_FILETYPE 0x0002

	#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
	#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE 0x0002
	#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR 0x0004

	#define EXT2_VALID_FS 1
	#define EXT2_ERROR_FS 2

	struct Ext2SuperBlock {
		uint32_t s_inodes_count;
		uint32_t s_blocks_count;
		uint32_t s_r_blocks_count;
		uint32_t s_free_blocks_count;
		uint32_t s_free_inodes_count;
		uint32_t s_first_data_block;
		uint32_t s_log_block_size;
		int32_t s_log_frag_size;
		uint32_t s_blocks_per_group;
		uint32_t s_frags_per_group;
		uint32_t s_inodes_per_group;
		uint32_t s_mtime;
		uint32_t s_wtime;
		uint16_t s_mnt_count;
		uint16_t s_max_mnt_count;
		uint16_t s_magic;
		uint16_t s_state;
		uint16_t s_errors;
		uint16_t s_minor_rev_level;
		uint32_t s_lastcheck;
		uint32_t s_checkinterval;
		uint32_t s_creator_os;
		uint32_t s_rev_level;
		uint16_t s_def_resuid;
		uint16_t s_def_resgid;

		/* EXT2_DYNAMIC_REV Specific: */
		uint32_t s_first_ino;
		uint16_t s_inode_size;
		uint16_t s_block_group_nr;
		uint32_t s_feature_compat;
		uint32_t s_feature_incompat;
		uint32_t s_feature_ro_compat;
		uint8_t s_uuid[16];
		char s_volume_name[16];
		char s_last_mounted[64];
		uint32_t s_algo_bitmap;

		/* Performance Hints: */
		uint8_t s_prealloc_blocks;
		uint8_t s_prealloc_dir_blocks;
		uint8_t alignment1[2];

		/* Journaling Support: */
		uint8_t s_journal_uuid[16];
		uint32_t s_journal_inum;
		uint32_t s_journal_dev;
		uint32_t s_last_orphan;

		/* Directory Indexing Support: */
		uint32_t s_hash_seed[4];
		uint8_t s_def_hash_version;
		uint8_t alignment2[3];

		/* Other options: */
		uint32_t s_default_mount_options;
		uint32_t s_first_meta_bg;
		uint8_t reserved[760];
	} __attribute__((packed));
	_Static_assert(sizeof(struct Ext2SuperBlock) == 1024, "Expecting Ext2Superblock with 1024 bytes.");

	struct Ext2BlockGroupDescriptor {
		uint32_t bg_block_bitmap;
		uint32_t bg_inode_bitmap;
		uint32_t bg_inode_table;
		uint16_t bg_free_blocks_count;
		uint16_t bg_free_inodes_count;
		uint16_t bg_used_dirs_count;
		uint16_t bg_pad;
		uint8_t bg_reserved[12];
	} __attribute__((packed));

	struct Ext2INode {
		uint16_t i_mode;
		uint16_t i_uid;
		uint32_t i_size;
		int32_t i_atime; /* The last time this inode was accessed. */
		int32_t i_ctime; /* The last time this inode metadata was changed. */
		int32_t i_mtime; /* The last time this inode was modified. */
		int32_t i_dtime;
		uint16_t i_gid;
		uint16_t i_links_count;
		uint32_t i_blocks;
		uint32_t i_flags;
		uint32_t i_osd1;
		uint32_t i_block[15];
		uint32_t i_generation;
		uint32_t i_file_acl;
		uint32_t i_dir_acl;
		uint32_t i_faddr;
		uint8_t i_osd2[12];
	} __attribute__((packed));

	struct Ext2LinkedDirectoryEntry {
		uint32_t inode;
		uint16_t rec_len;
		uint8_t name_len;
		uint8_t file_type;
		char nameFirstCharacter;
	} __attribute__((packed));

	struct Ext2FileSystem;

	struct Ext2VirtualFileSystemNode {
		struct VirtualFileSystemNode virtualFileSystemNode;
		struct Ext2FileSystem* fileSystem;
		struct Ext2INode iNode;
		uint32_t iNodeIndex;
		struct DoubleLinkedListElement availableListElement;
		bool isDirty;
	};
	_Static_assert(sizeof(struct Ext2VirtualFileSystemNode) <= PAGE_FRAME_SIZE, "The Ext2VirtualFileSystemNode must fit inside a page frame.");

	struct Ext2FileSystem {
		struct FileSystem fileSystem;
		struct VirtualFileSystemOperations operations;
		struct Ext2SuperBlock superBlock;
		struct BlockDevice* blockDevice;

		bool isSuperBlockOrBlockGroupsDirty;

		struct DoubleLinkedList blockGroupDescriptorsPageFrameList;

		struct DoubleLinkedList ext2VirtualFileSystemNodesPageFrameList;
		struct DoubleLinkedList availableExt2VirtualFileSystemNodesList;

		struct BTree ext2VirtualFileSystemNodeByINodeIndex;
		int ext2VirtualFileSystemNodeByINodeIndexMemoryReservationId;

		uint32_t blockSize;
		uint32_t blockGroupsCount;
		uint32_t blockGroupDescriptorsPerBlock;
		uint32_t iNodesPerBlock;
		uint32_t dataBlockIndexesPerBlock;
	};
	_Static_assert(sizeof(struct Ext2FileSystem) <= PAGE_FRAME_SIZE, "The Ext2FileSystem must fit inside a page frame.");

	APIStatusCode ext2FileSystemInitialize(struct Ext2FileSystem* ext2FileSystem, struct BlockDevice* blockDevice, uint32_t maxSimultaneouslyOpenINodes);

#endif
