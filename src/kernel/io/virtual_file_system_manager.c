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

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "kernel/api_status_code.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/process/process.h"
#include "kernel/io/open_file_description.h"
#include "kernel/io/mounted_file_system.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "util/double_linked_list.h"
#include "util/path_utils.h"
#include "util/string_stream_writer.h"
#include "util/string_utils.h"

#define OPEN_FILE_DESCRIPTIONS_PAGE_FRAME_COUNT 2

static struct DoubleLinkedList mountedFileSystemsList;
static struct DoubleLinkedList availableOpenFileDescriptionsList;
static struct DoubleLinkedList usedOpenFileDescriptionsList;

uint32_t virtualFileSystemManagerGetOpenFileDescriptionCount(void) {
	return (PAGE_FRAME_SIZE / sizeof(struct OpenFileDescription)) * OPEN_FILE_DESCRIPTIONS_PAGE_FRAME_COUNT;
}

APIStatusCode virtualFileSystemManagerValidateAndGetOpenFileDescription(int fileDescriptorIndex, struct Process* process, struct OpenFileDescription** openFileDescription) {
	APIStatusCode result;

	if (fileDescriptorIndex >= MAX_FILE_DESCRIPTORS_PER_PROCESS || fileDescriptorIndex < 0) {
		*openFileDescription = NULL;
		result = EBADF;
	} else {
		struct FileDescriptor* fileDescriptor = &process->fileDescriptors[fileDescriptorIndex];
		*openFileDescription = fileDescriptor->openFileDescription;
		if (*openFileDescription == NULL) {
			result = EBADF;
		} else {
			result = SUCCESS;
		}
	}

	return result;
}

APIStatusCode virtualFileSystemManagerInitialize(void) {
	APIStatusCode result = SUCCESS;

	doubleLinkedListInitialize(&mountedFileSystemsList);
	doubleLinkedListInitialize(&availableOpenFileDescriptionsList);
	doubleLinkedListInitialize(&usedOpenFileDescriptionsList);

	for (int i = 0; i < OPEN_FILE_DESCRIPTIONS_PAGE_FRAME_COUNT; i++) {
		struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
		if (doubleLinkedListElement == NULL) {
			result = ENOMEM;
		} else {
			struct OpenFileDescription *openFileDescriptions = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);

			for (int j = 0; j < PAGE_FRAME_SIZE / sizeof(struct OpenFileDescription); j++) {
				doubleLinkedListInsertAfterLast(&availableOpenFileDescriptionsList, &openFileDescriptions[j].doubleLinkedListElement);
			}
		}
	}

	return result;
}

static int compare(struct MountedFileSystem* mountedFileSystem1, struct MountedFileSystem* mountedFileSystem2) {
	int pathLength1 = strlen(mountedFileSystem1->path);
	int pathLength2 = strlen(mountedFileSystem2->path);

	if (pathLength1 != pathLength2) {
		return pathLength2 - pathLength1;

	} else {
		return strcmp(mountedFileSystem1->path, mountedFileSystem2->path);
	}
}

APIStatusCode virtualFileSystemManagerMountFileSystem(const char* mountPoint, struct FileSystem* fileSystem) {
	APIStatusCode result = SUCCESS;

	if (strlen(mountPoint) > MOUNTED_FILE_SYSTEM_PATH_MAX_LENGTH) {
		result = ENAMETOOLONG;

	} else {
		struct VirtualFileSystemNode* mountPointVirtualFileSystemNode;
		if (strcmp(mountPoint, "/") != 0) {
			struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
			if (doubleLinkedListElement != NULL) {
				struct PathUtilsContext* pathUtilsContext = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
				strcpy(pathUtilsContext->buffer, mountPoint);
				result = pathUtilsCalculatePathSegments(pathUtilsContext);
				if (result == SUCCESS) {
					result = virtualFileSystemManagerResolvePath(NULL, pathUtilsContext, false, false, 0, false, &mountPointVirtualFileSystemNode);
					if (result == SUCCESS) {
						assert(mountPointVirtualFileSystemNode->usageCount > 0);
						virtualFileSystemManagerReleaseNodeReservation(NULL, mountPointVirtualFileSystemNode, NULL);
					}
				}
				memoryManagerReleasePageFrame(doubleLinkedListElement, -1);

			} else {
				result = ENOMEM;
			}
		}
		if (result == SUCCESS) {
			struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
			if (doubleLinkedListElement == NULL) {
				result = ENOMEM;

			} else {
				struct MountedFileSystem* mountedFileSystem = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
				memset(mountedFileSystem, 0, sizeof(struct MountedFileSystem));
				strcpy(mountedFileSystem->path, mountPoint);
				int mountPointLength = strlen(mountPoint);
				if (mountPointLength > 1 && mountPoint[mountPointLength - 1] == '/') {
					mountedFileSystem->path[mountPointLength - 1] = '\0';
				}
				mountedFileSystem->fileSystem = fileSystem;
				doubleLinkedListInsertAfterLast(&mountedFileSystemsList, &mountedFileSystem->doubleLinkedListElement);
				/* Sort to guarantee that the more specific will be positioned first. */
				doubleLinkedListSort(&mountedFileSystemsList, (int (*)(struct DoubleLinkedListElement*, struct DoubleLinkedListElement*)) &compare);
			}
		}
	}

	return result;
}

APIStatusCode virtualFileSystemManagerCloseAllOpenFileDescriptions(void) {
	APIStatusCode result = SUCCESS;

	struct DoubleLinkedListElement* doubleLinkedListElement;
	while (doubleLinkedListSize(&usedOpenFileDescriptionsList) > 0) {
		doubleLinkedListElement = doubleLinkedListFirst(&usedOpenFileDescriptionsList);
		virtualFileSystemManagerCloseOpenFileDescription(NULL, (void*) doubleLinkedListElement);
	}

	return result;
}

APIStatusCode virtualFileSystemManagerUnmountAllFileSystems(void) {
	APIStatusCode result = SUCCESS;
	while (doubleLinkedListSize(&mountedFileSystemsList) > 0) {
		struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&mountedFileSystemsList);
		struct MountedFileSystem* mountedFileSystem = (void*) doubleLinkedListElement;
		if (mountedFileSystem->fileSystem->afterUnmount != NULL) {
			result = apiStatusCodeRetainFirstFailure(result, mountedFileSystem->fileSystem->afterUnmount(mountedFileSystem->fileSystem));
		}
		memoryManagerReleasePageFrame(memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) doubleLinkedListElement), -1);
	}

	return result;
}

#define MAX_RECURSION_DEPTH 5
APIStatusCode virtualFileSystemManagerResolvePath(struct Process* process, struct PathUtilsContext* pathUtilsContext,
		bool followLastSymbolicLink, bool createIfDoesNotExist, mode_t mode, bool failIfAlreadyExists,
		struct VirtualFileSystemNode** resolvedVirtualFileSystemNode) {
	assert(pathUtilsContext != NULL);

	int symbolicLinkRecursionDepth = 0;
	APIStatusCode result = SUCCESS;

	LOOP:
	*resolvedVirtualFileSystemNode = NULL;

	result = ENOENT;

	bool found = false;
	struct MountedFileSystem* mountedFileSystem = NULL;
	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&mountedFileSystemsList);
	while (doubleLinkedListElement != NULL) {
		mountedFileSystem = (void*) doubleLinkedListElement;

		if (stringUtilsStartsWith(pathUtilsContext->buffer, mountedFileSystem->path)) {
			found = true;
			break;
		}
		doubleLinkedListElement = doubleLinkedListElement->next;
	}

	if (found) {
		strcpy(pathUtilsContext->buffer, pathUtilsContext->buffer + strlen(mountedFileSystem->path));
		result = pathUtilsCalculatePathSegments(pathUtilsContext);

		if (result == SUCCESS) {
			if (mountedFileSystem->fileSystem->virtualFileSystemNode->operations->walk != NULL) {
				struct VirtualFileSystemNode* previousVirtualFileSystemNode = NULL;
				struct VirtualFileSystemNode* nextVirtualFileSystemNode = mountedFileSystem->fileSystem->virtualFileSystemNode;
				nextVirtualFileSystemNode->usageCount++;
				int segmentIndex = 0;
				bool created = false;
				do {
					const char* segment = NULL;
					if (segmentIndex >= pathUtilsContext->segmentCount) {
						segment = NULL;
					} else {
						segment = pathUtilsContext->segments[segmentIndex];
					}

					struct VirtualFileSystemOperations* operations;
					if (previousVirtualFileSystemNode != NULL) {
						virtualFileSystemManagerReleaseNodeReservation(process, previousVirtualFileSystemNode, NULL);
					}

					previousVirtualFileSystemNode = nextVirtualFileSystemNode;
					operations = nextVirtualFileSystemNode->operations;
					if (operations->walk != NULL) {
						result = operations->walk(nextVirtualFileSystemNode, process, segment,
								segmentIndex + 1 >= pathUtilsContext->segmentCount && createIfDoesNotExist, mode,
								&nextVirtualFileSystemNode, &created);
						assert(nextVirtualFileSystemNode == NULL || nextVirtualFileSystemNode->usageCount > 0);

						mode_t mode = 0;
						if (nextVirtualFileSystemNode != NULL) {
							mode = nextVirtualFileSystemNode->operations->getMode(nextVirtualFileSystemNode);
						}
						if (nextVirtualFileSystemNode != NULL && S_ISLNK(mode)
								&& (followLastSymbolicLink || segmentIndex + 1 < pathUtilsContext->segmentCount)) {
							if (symbolicLinkRecursionDepth + 1 <= MAX_RECURSION_DEPTH) {
								if (operations->mergeWithSymbolicLinkPath != NULL) {
									result = operations->mergeWithSymbolicLinkPath(nextVirtualFileSystemNode, pathUtilsContext, segmentIndex);
									if (result == SUCCESS) {
										result = pathUtilsCalculatePathSegments(pathUtilsContext);
									}
									if (result == SUCCESS) {
										pathUtilsNormalize(pathUtilsContext);
										pathUtilsCombinePathSegments(pathUtilsContext, true);
									}

									if (result == SUCCESS) {
										symbolicLinkRecursionDepth++;
										if (previousVirtualFileSystemNode != NULL && previousVirtualFileSystemNode != nextVirtualFileSystemNode) {
											virtualFileSystemManagerReleaseNodeReservation(process, previousVirtualFileSystemNode, NULL);
										}
										virtualFileSystemManagerReleaseNodeReservation(process, nextVirtualFileSystemNode, NULL);
										/* A new context has been established, restart the calculation. */
										goto LOOP;
									}

								} else {
									result = EPERM;
								}

							} else {
								result = ELOOP;
							}
						}

						segmentIndex++;

					} else {
						result = EPERM;
					}

				} while (segmentIndex < pathUtilsContext->segmentCount && nextVirtualFileSystemNode != NULL && result == SUCCESS);

				if (nextVirtualFileSystemNode != NULL && result == SUCCESS) {
					if (!created && failIfAlreadyExists) {
						virtualFileSystemManagerReleaseNodeReservation(process, nextVirtualFileSystemNode, NULL);
						result = EEXIST;
					} else {
						*resolvedVirtualFileSystemNode = nextVirtualFileSystemNode;
						assert(nextVirtualFileSystemNode->usageCount > 0);
					}

				} else if (nextVirtualFileSystemNode != NULL) {
					virtualFileSystemManagerReleaseNodeReservation(process, nextVirtualFileSystemNode, NULL);
				}

				if (previousVirtualFileSystemNode != NULL && previousVirtualFileSystemNode != nextVirtualFileSystemNode) {
					virtualFileSystemManagerReleaseNodeReservation(process, previousVirtualFileSystemNode, NULL);
				}

			} else {
				result = EPERM;
			}
		}
	}

	return result;
}

void virtualFileSystemManagerReleaseOpenFileDescription(struct OpenFileDescription* openFileDescription) {
	struct DoubleLinkedListElement* doubleLinkedListElement = (struct DoubleLinkedListElement*) openFileDescription;
	assert(!doubleLinkedListContainsFoward(&availableOpenFileDescriptionsList, doubleLinkedListElement));
	assert(doubleLinkedListContainsFoward(&usedOpenFileDescriptionsList, doubleLinkedListElement));
	doubleLinkedListRemove(&usedOpenFileDescriptionsList, doubleLinkedListElement);
	doubleLinkedListInsertAfterLast(&availableOpenFileDescriptionsList, doubleLinkedListElement);
}

struct OpenFileDescription* virtualFileSystemManagerAcquireOpenFileDescription(void) {
	if (doubleLinkedListSize(&availableOpenFileDescriptionsList) > 0) {
		struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&availableOpenFileDescriptionsList);
		struct OpenFileDescription* openFileDescription = (struct OpenFileDescription*) doubleLinkedListElement;
		memset(openFileDescription, 0, sizeof(struct OpenFileDescription));
		doubleLinkedListInsertAfterLast(&usedOpenFileDescriptionsList, doubleLinkedListElement);
		return openFileDescription;

	} else {
		return NULL;
	}
}

APIStatusCode virtualFileSystemManagerCloseOpenFileDescription(struct Process* currentProcess, struct OpenFileDescription* openFileDescription) {
	APIStatusCode result = SUCCESS;
	struct VirtualFileSystemNode* virtualFileSystemNode = NULL;

	if (openFileDescription == NULL) {
		result = EBADF;

	} else {
		assert(openFileDescription->usageCount > 0);

		virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
		assert(virtualFileSystemNode->usageCount > 0);

		openFileDescription->usageCount--;

		if (openFileDescription->usageCount == 0) {
			virtualFileSystemManagerReleaseNodeReservation(currentProcess, virtualFileSystemNode, openFileDescription);
			virtualFileSystemManagerReleaseOpenFileDescription(openFileDescription);
		}
	}

	return result;
}

void virtualFileSystemManagerReleaseNodeReservation(struct Process* currentProcess, struct VirtualFileSystemNode* virtualFileSystemNode, struct OpenFileDescription* openFileDescription) {
	assert(virtualFileSystemNode != NULL);

	struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

	assert(virtualFileSystemNode->usageCount > 0);
	virtualFileSystemNode->usageCount--;
	if (operations->afterNodeReservationRelease != NULL) {
		operations->afterNodeReservationRelease(virtualFileSystemNode, currentProcess, openFileDescription);
	}
}

APIStatusCode virtualFileSystemManagerPrintDebugReport(void) {
	int bufferSize = 512;
	char buffer[bufferSize];
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);
	streamWriterFormat(&stringStreamWriter.streamWriter, "Virtual file system manager report:\n");
	streamWriterFormat(&stringStreamWriter.streamWriter, "  availableOpenFileDescriptionsList=%d\n", doubleLinkedListSize(&availableOpenFileDescriptionsList));
	streamWriterFormat(&stringStreamWriter.streamWriter, "  usedOpenFileDescriptionsList=%d\n", doubleLinkedListSize(&usedOpenFileDescriptionsList));
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	logDebug("%s", buffer);

	return SUCCESS;
}
