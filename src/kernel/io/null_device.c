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

#include "kernel/io/null_device.h"
#include "kernel/io/virtual_file_system_operations.h"
#include "kernel/io/virtual_file_system_node.h"

static struct VirtualFileSystemOperations nullDeviceVirtualFileSystemOperations;
static struct VirtualFileSystemNode nullDeviceVirtualFileSystemNode;

static APIStatusCode open(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription** openFileDescription, int flags) {
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);
	return SUCCESS;
}

static APIStatusCode read(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);
	*count = 0;
	return SUCCESS;
}

static APIStatusCode write(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);
	*count = bufferSize;
	return SUCCESS;
}

static mode_t getMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);
	return S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO;
}

static APIStatusCode status(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);

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
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);
	return ALWAYS_REPOSITION_TO_ZERO;
}

static off_t getSize(struct VirtualFileSystemNode* virtualFileSystemNode) {
	assert(virtualFileSystemNode == &nullDeviceVirtualFileSystemNode);

	return 0;
}

void nullDeviceInitialize(void) {
	memset(&nullDeviceVirtualFileSystemNode, 0, sizeof(struct VirtualFileSystemNode));
	nullDeviceVirtualFileSystemNode.operations = &nullDeviceVirtualFileSystemOperations;

	memset(&nullDeviceVirtualFileSystemOperations, 0, sizeof(struct VirtualFileSystemOperations));
	nullDeviceVirtualFileSystemOperations.open = &open;
	nullDeviceVirtualFileSystemOperations.read = &read;
	nullDeviceVirtualFileSystemOperations.write = &write;
	nullDeviceVirtualFileSystemOperations.getMode = &getMode;
	nullDeviceVirtualFileSystemOperations.status = &status;
	nullDeviceVirtualFileSystemOperations.getOpenFileDescriptionOffsetRepositionPolicy = &getOpenFileDescriptionOffsetRepositionPolicy;
	nullDeviceVirtualFileSystemOperations.getSize = &getSize;

	devicesFileSystemRegisterDevice(&nullDeviceVirtualFileSystemNode, "null");
}
