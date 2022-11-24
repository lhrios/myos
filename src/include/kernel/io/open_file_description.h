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

#ifndef KERNEL_OPEN_FILE_DESCRIPTION_H
	#define KERNEL_OPEN_FILE_DESCRIPTION_H

	#include <sys/types.h>

	#include "kernel/io/virtual_file_system_node.h"

	#include "util/double_linked_list.h"

	struct OpenFileDescription {
		struct DoubleLinkedListElement doubleLinkedListElement;
		struct VirtualFileSystemNode* virtualFileSystemNode;
		uint32_t usageCount;
		off_t offset;
		int flags;
	};
	_Static_assert(sizeof(struct OpenFileDescription) <= PAGE_FRAME_SIZE, "The OpenFileDescription must fit inside a page frame.");

#endif
