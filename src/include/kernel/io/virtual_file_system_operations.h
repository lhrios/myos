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

#ifndef KERNEL_VIRTUAL_FILE_SYSTEM_OPERATIONS_H
	#define KERNEL_VIRTUAL_FILE_SYSTEM_OPERATIONS_H

	#include <dirent.h>
	#include <poll.h>
	#include <stdint.h>
	#include <stdlib.h>
	#include <stdarg.h>

	#include <sys/stat.h>

	#include "kernel/api_status_code.h"

	#include "util/double_linked_list.h"
	#include "util/path_utils.h"

	enum OpenFileDescriptionOffsetRepositionPolicy {
		REPOSITION_NOT_ALLOWED,
		ALWAYS_REPOSITION_TO_ZERO,
		REPOSITION_BASED_ON_FILE_SIZE,
		REPOSITION_FREELY
	};

	struct OpenFileDescription;
	struct IOEventMonitoringContext;
	struct Process;
	struct VirtualFileSystemNode;

	struct VirtualFileSystemOperations {
		APIStatusCode (*walk)(struct VirtualFileSystemNode*, struct Process*, const char*, bool, mode_t, struct VirtualFileSystemNode**, bool*);
		APIStatusCode (*open)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription**, int);
		APIStatusCode (*read)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, void*, size_t, size_t*);
		APIStatusCode (*write)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, void*, size_t, size_t*);
		void (*afterNodeReservationRelease)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*);
		APIStatusCode (*readDirectoryEntry)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, struct dirent*, bool*);
		APIStatusCode (*status)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, struct stat*);
		enum OpenFileDescriptionOffsetRepositionPolicy (*getOpenFileDescriptionOffsetRepositionPolicy)(struct VirtualFileSystemNode*);
		APIStatusCode (*mergeWithSymbolicLinkPath)(struct VirtualFileSystemNode*, struct PathUtilsContext*, int lastPathSegmentIndex);
		mode_t (*getMode)(struct VirtualFileSystemNode*); /* Required */
		off_t (*getSize)(struct VirtualFileSystemNode*);
		APIStatusCode (*changeFileSize)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, off_t);
		APIStatusCode (*createDirectory)(struct VirtualFileSystemNode*, struct Process*, const char*, mode_t);
		APIStatusCode (*createName)(struct VirtualFileSystemNode*, struct Process*, struct VirtualFileSystemNode*, const char*);
		APIStatusCode (*releaseName)(struct VirtualFileSystemNode*, struct Process*, const char*);
		APIStatusCode (*releaseDirectory)(struct VirtualFileSystemNode*, struct Process*, struct VirtualFileSystemNode*);
		APIStatusCode (*createSymbolicLink)(struct VirtualFileSystemNode*, struct Process*, const char*, const char*, size_t);
		APIStatusCode (*manipulateDeviceParameters)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, uint32_t*);
		APIStatusCode (*manipulateOpenFileDescriptionParameters)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, uint32_t*);
		APIStatusCode (*monitorIoEvent)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*,
				struct IOEventMonitoringContext* ioEventMonitoringContext);
		APIStatusCode (*startIoEventMonitoring)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*,
				struct IOEventMonitoringContext* ioEventMonitoringContext);
		void (*stopIoEventMonitoring)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*,
				struct IOEventMonitoringContext* ioEventMonitoringContext);
		APIStatusCode (*getDirectoryParent)(struct VirtualFileSystemNode*, struct Process*, struct VirtualFileSystemNode**);
		APIStatusCode (*rename)(struct VirtualFileSystemNode*, struct Process*, struct VirtualFileSystemNode*, const char*,
				struct VirtualFileSystemNode*, struct VirtualFileSystemNode*, const char*);
		struct FileSystem* (*getFileSystem)(struct VirtualFileSystemNode*);
	};

#endif
