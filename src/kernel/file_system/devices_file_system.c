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

#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include <myos.h>

#include "kernel/api_status_code.h"
#include "kernel/cmos.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/process/process.h"

#include "kernel/file_system/devices_file_system.h"
#include "kernel/file_system/file_system.h"

#include "kernel/io/open_file_description.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "util/path_utils.h"

#define DEVICE_FILE_SYSTEM_NODE_NAME_MAX_LENGTH 31
struct DeviceFileSystemNode {
	struct VirtualFileSystemNode* virtualFileSystemNode;
	char name[DEVICE_FILE_SYSTEM_NODE_NAME_MAX_LENGTH + 1];
};

#define DEVICE_FILE_SYSTEM_ROOT_MAX_CHILDREN ((PAGE_FRAME_SIZE - sizeof(struct VirtualFileSystemOperations) - sizeof(uint32_t) \
	- sizeof(struct VirtualFileSystemNode) - sizeof(mode_t) - sizeof(struct FileSystem)) / (sizeof(struct DeviceFileSystemNode)))
_Static_assert(DEVICE_FILE_SYSTEM_ROOT_MAX_CHILDREN > 0, "There must be at least one child.");

struct DeviceFileSystemRoot {
	struct VirtualFileSystemNode virtualFileSystemNode;
	mode_t mode;
	uint32_t childrenCount;
	struct DeviceFileSystemNode children[DEVICE_FILE_SYSTEM_ROOT_MAX_CHILDREN];
};

struct DeviceFileSystem {
	struct FileSystem fileSystem;
	struct DeviceFileSystemRoot root;
	struct VirtualFileSystemOperations operations;
};
_Static_assert(sizeof(struct DeviceFileSystem) <= PAGE_FRAME_SIZE, "The DeviceFileSystemRoot must fit inside a page frame.");

static struct DeviceFileSystem* fileSystem;

static mode_t getMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	assert(virtualFileSystemNode == (struct VirtualFileSystemNode*) &fileSystem->root);

	struct DeviceFileSystemRoot* deviceFileSystemRoot = (struct DeviceFileSystemRoot*) virtualFileSystemNode;
	return deviceFileSystemRoot->mode;
}

static APIStatusCode status(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	assert(virtualFileSystemNode == (struct VirtualFileSystemNode*) &fileSystem->root);

	struct DeviceFileSystemRoot* root = &fileSystem->root;

	statInstance->st_atime = cmosGetInitializationTime();
	statInstance->st_blksize = PAGE_FRAME_SIZE;
	statInstance->st_blocks = 1;
	statInstance->st_ctime = cmosGetInitializationTime();
	statInstance->st_dev = DEVICES_FILE_SYSTEM_ID;
	statInstance->st_ino = 0;
	statInstance->st_mtime = cmosGetInitializationTime();
	statInstance->st_nlink = root->childrenCount + 1;
	statInstance->st_size = PAGE_FRAME_SIZE;

	statInstance->st_atim.tv_sec = statInstance->st_atime;
	statInstance->st_mtim.tv_sec = statInstance->st_mtime;
	statInstance->st_ctim.tv_sec = statInstance->st_ctime;

	return SUCCESS;
}

static APIStatusCode walk(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process,
		const char* pathSegment, bool createIfDoesNotExist, mode_t mode, struct VirtualFileSystemNode** nextVirtualFileSystemNode, bool* created) {
	assert(virtualFileSystemNode == (struct VirtualFileSystemNode*) &fileSystem->root);
	assert(virtualFileSystemNode->usageCount > 0);

	APIStatusCode result = ENOENT;
	*nextVirtualFileSystemNode = NULL;

	if (pathSegment == NULL) {
		result = SUCCESS;
		*nextVirtualFileSystemNode = virtualFileSystemNode;

	} else {
		size_t pathSegmentLength = strlen(pathSegment);
		assert(pathSegmentLength > 0);

		struct DeviceFileSystemRoot* root = &fileSystem->root;

		for (int i = 0; i < root->childrenCount; i++) {
			struct DeviceFileSystemNode *deviceFileSystemNode = &root->children[i];
			if (pathSegmentLength == strlen(deviceFileSystemNode->name) && strncmp(deviceFileSystemNode->name, pathSegment, pathSegmentLength) == 0) {
				*nextVirtualFileSystemNode = deviceFileSystemNode->virtualFileSystemNode;
				(*nextVirtualFileSystemNode)->usageCount++;
				result = SUCCESS;
				break;
			}
		}
	}

	if (created != NULL) {
		*created = false;
	}

	return result;
}

static APIStatusCode open(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process,
		struct OpenFileDescription** openFileDescription, int flags) {
	return SUCCESS;
}

static APIStatusCode readDirectoryEntry(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process,
		struct OpenFileDescription* openFileDescription, struct dirent* direntInstance, bool* endOfDirectory) {
	struct DeviceFileSystemRoot* root = &fileSystem->root;
	assert(virtualFileSystemNode == (struct VirtualFileSystemNode*) root);

	if (openFileDescription->offset < root->childrenCount + 2) {
		direntInstance->d_ino = (uint32_t) openFileDescription->offset;
		if (openFileDescription->offset < 2) {
			switch (openFileDescription->offset) {
				case 0:
					strcpy(direntInstance->d_name, ".");
					break;

				case 1:
					strcpy(direntInstance->d_name, "..");
					break;

				default:
					assert(false);
					break;
			}

		} else {
			struct DeviceFileSystemNode *deviceFileSystemNode = &root->children[openFileDescription->offset - 2];
			direntInstance->d_ino = (uint32_t) openFileDescription->offset;
			strcpy(direntInstance->d_name, deviceFileSystemNode->name);
		}

		openFileDescription->offset++;
		*endOfDirectory = false;

	} else {
		*endOfDirectory = true;
	}
	return SUCCESS;
}

APIStatusCode devicesFileSystemInitialize(void) {
	APIStatusCode result = SUCCESS;

	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement == NULL) {
		result = ENOMEM;

	} else {
		fileSystem = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		fileSystem->fileSystem.virtualFileSystemNode = &fileSystem->root.virtualFileSystemNode;
		fileSystem->fileSystem.afterUnmount = NULL;

		struct VirtualFileSystemOperations* operations = &fileSystem->operations;
		memset(operations, 0, sizeof(struct VirtualFileSystemOperations));
		operations->walk = &walk;
		operations->open = &open;
		operations->readDirectoryEntry = &readDirectoryEntry;
		operations->getMode = &getMode;
		operations->status = &status;

		struct DeviceFileSystemRoot* root = &fileSystem->root;
		memset(root, 0, sizeof(struct DeviceFileSystemRoot));
		root->mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		root->virtualFileSystemNode.operations = operations;
		root->virtualFileSystemNode.usageCount++;
	}

	return result;
}

APIStatusCode devicesFileSystemMount(const char* path) {
	return virtualFileSystemManagerMountFileSystem(path, &fileSystem->fileSystem);
}

bool devicesFileSystemRegisterDevice(struct VirtualFileSystemNode* virtualFileSystemNode, const char* name) {
	assert(strlen(name) <= DEVICE_FILE_SYSTEM_NODE_NAME_MAX_LENGTH);

	struct DeviceFileSystemRoot* root = &fileSystem->root;

	if (root->childrenCount + 1 > DEVICE_FILE_SYSTEM_ROOT_MAX_CHILDREN) {
		return false;
	} else {
		struct DeviceFileSystemNode *deviceFileSystemNode = &root->children[root->childrenCount++];
		deviceFileSystemNode->virtualFileSystemNode = virtualFileSystemNode;
		strcpy(deviceFileSystemNode->name, name);
		return true;
	}
}

struct VirtualFileSystemNode* devicesFileSystemGetDeviceVirtualFileSystemNodeByName(const char* deviceName) {
	struct DeviceFileSystemRoot* root = &fileSystem->root;
	for (int i = 0; i < root->childrenCount; i++) {
		struct DeviceFileSystemNode *deviceFileSystemNode = &root->children[i];
		if (strcmp(deviceFileSystemNode->name, deviceName) == 0) {
			return deviceFileSystemNode->virtualFileSystemNode;
		}
	}

	return NULL;
}
