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

#ifndef KERNEL_VIRTUAL_FILE_SYSTEM_MANAGER_H
	#define KERNEL_VIRTUAL_FILE_SYSTEM_MANAGER_H

	#include <stdbool.h>

	#include "kernel/memory_manager.h"

	#include "kernel/file_system/file_system.h"

	#include "kernel/io/virtual_file_system_node.h"

	#include "util/double_linked_list.h"
	#include "util/path_utils.h"

	APIStatusCode virtualFileSystemManagerInitialize(void);
	APIStatusCode virtualFileSystemManagerMountFileSystem(const char*, struct FileSystem*);
	APIStatusCode virtualFileSystemManagerResolvePath(struct Process* process, struct PathUtilsContext* pathUtilsContext,
			bool stopOnFirstSymbolicLink, bool createIfDoesNotExist, mode_t mode, bool failIfAlreadyExists,
			struct VirtualFileSystemNode** resolvedVirtualFileSystemNode);
	struct OpenFileDescription* virtualFileSystemManagerAcquireOpenFileDescription(void);
	void virtualFileSystemManagerReleaseOpenFileDescription(struct OpenFileDescription*);
	APIStatusCode virtualFileSystemManagerCloseOpenFileDescription(struct Process* currentProcess, struct OpenFileDescription*);
	APIStatusCode virtualFileSystemManagerValidateAndGetOpenFileDescription(int, struct Process*, struct OpenFileDescription**);
	APIStatusCode virtualFileSystemManagerCalculateNewOffset(int64_t, off_t, off_t, int, int64_t*);
	uint32_t virtualFileSystemManagerGetOpenFileDescriptionCount(void);
	void virtualFileSystemManagerReleaseNodeReservation(struct Process* currentProcess, struct VirtualFileSystemNode*, struct OpenFileDescription*);
	APIStatusCode virtualFileSystemManagerCloseAllOpenFileDescriptions(void);
	APIStatusCode virtualFileSystemManagerUnmountAllFileSystems(void);
	APIStatusCode virtualFileSystemManagerPrintDebugReport(void);

#endif
