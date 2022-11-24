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

#ifndef DIRENT_H
	#define DIRENT_H

	#include <sys/types.h>

	#include "kernel/limits.h"

	typedef struct {
		int fileDescriptorIndex;
	} DIR;

	struct dirent {
		ino_t d_ino; /* Inode number */
		char d_name[FILE_NAME_MAX_LENGTH]; /* Null-terminated filename */
	};

	DIR* opendir(const char* path);
	DIR* fdopendir(int fileDescriptorIndex);
	struct dirent* readdir(DIR* directory);
	void rewinddir(DIR* directory);
	int closedir(DIR* directory);
	int dirfd(DIR* directory);

#endif
