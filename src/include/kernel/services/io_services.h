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

#ifndef KERNEL_IO_UTILS_H
	#define KERNEL_IO_UTILS_H

	#include <poll.h>
	#include <stdint.h>

	#include <sys/types.h>

	#include "kernel/api_status_code.h"

	#include "kernel/io/virtual_file_system_node.h"

	APIStatusCode ioServicesCalculateNewOffset(int64_t fileSize, off_t currentOffset, off_t offset, int whence, int64_t* newOffset);

	APIStatusCode ioServicesOpen(struct Process* process, bool verifyUserAddress, const char* path, bool isPathNormalized, int flags, mode_t mode, int* fileDescriptorIndex);
	APIStatusCode ioServicesRead(struct Process* process, int fileDescriptorIndex, bool verifyUserAddress, void* buffer, size_t bufferSize, size_t* count);
	APIStatusCode ioServicesWrite(struct Process* process, int fileDescriptorIndex, bool verifyUserAddress, void* buffer, size_t bufferSize, size_t* count);
	APIStatusCode ioServicesStatus(struct Process* process, int fileDescriptorIndex, bool verifyUserAddress, struct stat* statInstance);
	APIStatusCode ioServicesChangeFileSize(struct Process* process, int fileDescriptorIndex, off_t newSize);
	APIStatusCode ioServicesCreateDirectory(struct Process* process, const char* newDirectoryPath, bool verifyUserAddress, mode_t mode);
	APIStatusCode ioServicesClose(struct Process* process, int fileDescriptorIndex);
	APIStatusCode ioServicesCreateName(struct Process* process, const char* targetPath, const char* namePathToCreate, bool verifyUserAddress);
	APIStatusCode ioServicesReleaseName(struct Process* process, const char* namePathToRelease, bool verifyUserAddress);
	APIStatusCode ioServicesReleaseDirectory(struct Process* process, const char* directoryPathToRelease, bool verifyUserAddress);
	APIStatusCode ioServicesCreateSymbolicLink(struct Process* process, const char* targetPath, const char* symbolicLinkPathToCreate, bool verifyUserAddress);
	APIStatusCode ioServicesRepositionOpenFileDescriptionOffset(struct Process* process, int fileDescriptorIndex, off_t offset, int whence, off_t* newOffset);
	APIStatusCode ioServicesSetCurrentWorkingDirectory(struct Process* process, char* newCurrentWorkingDirectory);
	APIStatusCode ioServicesChangeDeviceParameters(struct Process* process, int fileDescriptorIndex, uint32_t* request, bool verifyUserAddress);
	APIStatusCode ioServicesChangeOpenFileDescriptionParameters(struct Process* process, int fileDescriptorIndex, uint32_t* command, bool verifyUserAddress);
	APIStatusCode ioServicesChangeFileDescriptorParameters(struct Process* process, int fileDescriptorIndex, uint32_t* command, bool verifyUserAddress);
	APIStatusCode ioServicesReadDirectoryEntry(struct Process* process, int fileDescriptorIndex, struct dirent* direntInstance, bool verifyUserAddress, bool* endOfDirectory);
	APIStatusCode ioServicesDuplicateFileDescriptor(struct Process* process, int existentFileDescriptorIndex,
			int minimumFileDescriptorIndex, int* newFileDescriptorIndex, bool verifyUserAddress);
	APIStatusCode ioServicesFindLowestAvailableFileDescriptorIndex(struct Process* process, int minimumFileDescriptorIndex, int* fileDescriptorIndex);
	APIStatusCode ioServicesRename(struct Process* process, const char* oldPath, const char* newPath, bool verifyUserAddress);
	APIStatusCode ioServicesMonitorIOEvents(struct Process* process, struct pollfd* userIOEventMonitoringContexts, nfds_t ioEventMonitoringContextCount, int timeout, int* triggeredEventsCount);

#endif
