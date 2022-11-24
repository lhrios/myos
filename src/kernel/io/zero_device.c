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

#include <string.h>

#include <myos.h>

#include "kernel/cmos.h"

#include "kernel/file_system/devices_file_system.h"

#include "kernel/io/virtual_file_system_operations.h"
#include "kernel/io/virtual_file_system_node.h"
#include "kernel/io/zero_device.h"

static struct VirtualFileSystemOperations zeroDeviceVirtualFileSystemOperations;
static struct VirtualFileSystemNode zeroDeviceVirtualFileSystemNode;

static APIStatusCode open(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription** openFileDescription, int flags) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);
	return SUCCESS;
}

static APIStatusCode read(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);
	memset(buffer, 0, bufferSize);
	*count = bufferSize;
	return SUCCESS;
}

static APIStatusCode write(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);
	*count = bufferSize;
	return SUCCESS;
}

static mode_t getMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);
	return S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO;
}

static APIStatusCode status(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);

	statInstance->st_size = 0;
	statInstance->st_dev = ZERO_DEVICE_ID;
	statInstance->st_ino = 1;
	statInstance->st_atime = cmosGetInitializationTime();
	statInstance->st_ctime = cmosGetInitializationTime();
	statInstance->st_mtime = cmosGetInitializationTime();
	statInstance->st_rdev = myosCalculateUniqueId(statInstance->st_dev, statInstance->st_ino);
	statInstance->st_nlink = 1;

	return SUCCESS;
}

static enum OpenFileDescriptionOffsetRepositionPolicy getOpenFileDescriptionOffsetRepositionPolicy(struct VirtualFileSystemNode* virtualFileSystemNode) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);
	return ALWAYS_REPOSITION_TO_ZERO;
}

static off_t getSize(struct VirtualFileSystemNode* virtualFileSystemNode) {
	assert(virtualFileSystemNode == &zeroDeviceVirtualFileSystemNode);

	return 0;
}

void zeroDeviceInitialize(void) {
	memset(&zeroDeviceVirtualFileSystemNode, 0, sizeof(struct VirtualFileSystemNode));
	zeroDeviceVirtualFileSystemNode.operations = &zeroDeviceVirtualFileSystemOperations;

	memset(&zeroDeviceVirtualFileSystemOperations, 0, sizeof(struct VirtualFileSystemOperations));
	zeroDeviceVirtualFileSystemOperations.open = &open;
	zeroDeviceVirtualFileSystemOperations.read = &read;
	zeroDeviceVirtualFileSystemOperations.write = &write;
	zeroDeviceVirtualFileSystemOperations.getMode = &getMode;
	zeroDeviceVirtualFileSystemOperations.status = &status;
	zeroDeviceVirtualFileSystemOperations.getOpenFileDescriptionOffsetRepositionPolicy = &getOpenFileDescriptionOffsetRepositionPolicy;
	zeroDeviceVirtualFileSystemOperations.getSize = &getSize;

	devicesFileSystemRegisterDevice(&zeroDeviceVirtualFileSystemNode, "zero");
}
