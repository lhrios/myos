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

#ifndef FCNTL_H
	#define FCNTL_H

	#define O_DO_NOT_FOLLOW_LAST_SYMBOLIC_LINK 0x00080000 /* MyOS specific. It allows opening a symbolic link. */

	#define O_APPEND 0x1
	#define O_CREAT 0x2
	#define O_TRUNC 0x4
	#define O_RDWR 0x8
	#define O_WRONLY 0x10
	#define O_RDONLY 0x20
	#define O_CLOEXEC 0x40
	#define O_DIRECTORY 0x80
	#define O_PATH 0x100 // TODO: Implement me!
	#define O_NONBLOCK 0x200 // TODO: Implement me!
	#define O_EXCL 0x400
	#define O_NOCTTY 0x800

	#ifndef KERNEL_CODE
		#include <sys/types.h>

		int open(const char* path, int flags, ...);
	#endif

	#define F_DUPFD 0 /* Duplicate file descriptor. */
	#define F_GETFD 1 /* Get file descriptor flags. */
	#define F_SETFD 2 /* Set file descriptor flags. */
	#define F_GETFL 3 /* Get file status flags and file access modes. */
	#define F_SETFL 4 /* Set file status flags. */
	#define FD_CLOEXEC 1 /* The file descriptor will automatically be closed during a successful execve. */

   // TODO: Implement me!
	#define F_DUPFD_CLOEXEC 1030 /* Duplicate file descriptor with the close-on-exec flag FD_CLOEXEC set. */
	#define F_GETLK 5 /* Get record locking information. */
	#define F_SETLK 6 /* Set record locking information. */
	#define F_SETLKW 7 /* Set record locking information; wait if blocked. */
	#define F_GETOWN 9 /* Get process or process group ID to receive SIGURG signals. */
	#define F_SETOWN 8 /* Set process or process group ID to receive SIGURG signals. */

	int fcntl(int fileDescriptorIndex, int command, ...);

#endif
