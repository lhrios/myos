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

#ifndef SYS_STAT_H
	#define SYS_STAT_H

	#include <time.h>

	#include <sys/types.h>

	#define S_IFMT 0xF000 /* bit mask for the file type bit field */

	#define S_IFSOCK 0xC000 /* Socket */
	#define S_IFLNK 0xA000 /* Symbolic link */
	#define S_IFREG 0x8000 /* Regular file */
	#define S_IFBLK 0x6000 /* Block device */
	#define S_IFDIR 0x4000 /* Directory */
	#define S_IFCHR 0x2000 /* Character device */
  	#define S_IFIFO 0x1000 /* FIFO */

	#ifdef KERNEL_CODE
		#define S_IS_MOUNTABLE_BLOCK_DEVICE 0x80000000
	#endif

	#define S_ISREG(m) ((m & S_IFMT) == S_IFREG)
	#define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
	#define S_ISCHR(m) ((m & S_IFMT) == S_IFCHR)
	#define S_ISBLK(m) ((m & S_IFMT) == S_IFBLK)
	#define S_ISFIFO(m) ((m & S_IFMT) == S_IFIFO)
	#define S_ISLNK(m) ((m & S_IFMT) == S_IFLNK)
	#define S_ISSOCK(m) ((m & S_IFMT) == S_IFSOCK)

	/*
	 * References:
	 * - https://publications.opengroup.org/
	 * - https://man7.org/linux/man-pages/dir_section_3.html
	 */

	struct stat {
		/* The st_ino and st_dev fields taken together uniquely identify the file within the system. */
		dev_t st_dev; /* Device ID of device containing file. */
		ino_t st_ino; /* File serial number. */

		mode_t st_mode; /* Mode of file. */
		nlink_t st_nlink; /* Number of hard links to the file. */
		uid_t st_uid; /* User ID of file. */
		gid_t st_gid; /* Group ID of file. */
		dev_t st_rdev; /* Device ID (if file is character or block special). */

		/*
		 * For regular files, the file size in bytes.
		 * For symbolic links, the length in bytes of the path contained in the symbolic link.
		 * For a shared memory object, the length in bytes.
		 * For a typed memory object, the length in bytes.
		 * For other file types, the use of this field is unspecified.
		 */
		off_t st_size;
		blksize_t st_blksize; /* A file system-specific preferred I/O block size for this object. In some file system types, this may vary from file to file. */
		blkcnt_t st_blocks; /* Number of blocks allocated for this object. */
		time_t st_atime; /* Last data access timestamp. */
		time_t st_mtime; /* Last data modification timestamp. */
		time_t st_ctime; /* Last file status change timestamp. */

      struct timespec st_atim; /* Time of last access. */
      struct timespec st_mtim; /* Time of last modification. */
      struct timespec st_ctim; /* Time of last status change. */
	};

	int fstat(int fileDescriptorIndex, struct stat* statInstance);
	int stat(const char* path, struct stat* statInstance);
	int lstat(const char* path, struct stat* statInstance);

	int mkdir(const char* path, mode_t mode);
	int mknod(const char* path, mode_t mode, dev_t device); // TODO: Implement me!

	#define S_ISUID 04000
	#define S_ISGID 02000
	#define S_ISVTX 01000

	/* Owner: */
	#define S_IRWXU 00700
	#define S_IRUSR 00400
	#define S_IWUSR 00200
	#define S_IXUSR 00100

	/* Group: */
	#define S_IRWXG 00070
	#define S_IRGRP 00040
	#define S_IWGRP 00020
	#define S_IXGRP 00010

	/* Other users: */
	#define S_IRWXO 00007
	#define S_IROTH 00004
	#define S_IWOTH 00002
	#define S_IXOTH 00001

	mode_t umask(mode_t mask);
	int chmod(const char* path, mode_t mode); // TODO: Implement me!
	int chown(const char *path, uid_t owner, gid_t group); // TODO: Implement me!

#endif
