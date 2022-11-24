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

#ifndef KERNEL_MOUNTED_FILE_SYSTEM_H
	#define KERNEL_MOUNTED_FILE_SYSTEM_H

	#include "kernel/io/virtual_file_system_node.h"

	#include "util/double_linked_list.h"

	#define MOUNTED_FILE_SYSTEM_PATH_MAX_LENGTH 1023

	struct MountedFileSystem {
		struct DoubleLinkedListElement doubleLinkedListElement;
		char path[MOUNTED_FILE_SYSTEM_PATH_MAX_LENGTH + 1];
		struct FileSystem* fileSystem;
	};

#endif
