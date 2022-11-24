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

#ifndef KERNEL_LIMITS_H
	#define KERNEL_LIMITS_H

	#define FILE_NAME_MAX_LENGTH 256 /* # chars in a file name including the terminating null byte ('\0') */
	#define PATH_MAX_LENGTH 2048 /* # chars in a path name including the terminating null byte ('\0') */
	#define FILE_MAX_SIZE 0x7FFFFFFF
	#define MAX_FILE_DESCRIPTORS_PER_PROCESS 32
	#define DATA_SEGMENT_MAX_SIZE (1024 * 1024 * 1024 * 1)

#endif
