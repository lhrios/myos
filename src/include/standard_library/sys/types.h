/*
 * Copyright 2022 Luis Henrique O. Rios
 *
 * This file is part of MyOS.
 *
 * MyOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MyOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyOS. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SYS_TYPES_H
	#define SYS_TYPES_H

	#include <stddef.h>
	#include <stdint.h>

	typedef int32_t pid_t;
	typedef uint32_t ino_t;
	typedef int32_t off_t;
	typedef uint32_t dev_t;
	typedef int32_t blksize_t;
	typedef int32_t blkcnt_t;
	typedef uint32_t mode_t;
	typedef uint32_t nlink_t;
	typedef uint32_t uid_t;
	typedef uint32_t gid_t;
	typedef int32_t time_t;
	typedef int32_t ssize_t;
	typedef int32_t suseconds_t;

#endif
