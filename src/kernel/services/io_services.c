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

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include <sys/stat.h>

#include "standard_library_implementation/file_descriptor_offset_reposition_constants.h"

#include "kernel/command_scheduler.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/process/process_manager.h"

#include "kernel/io/open_file_description.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "kernel/services/io_services.h"
#include "kernel/services/process_services.h"

#include "util/path_utils.h"

static APIStatusCode parsePath(struct Process* process, bool verifyUserAddress,
		const char* path, bool includeLastPathSegment, bool isPathNormalized, struct PathUtilsContext** pathUtilsContext) {
	APIStatusCode result = SUCCESS;

	struct PathUtilsContext* localPathUtilsContext = *pathUtilsContext;
	if (localPathUtilsContext == NULL) {
		struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
		if (doubleLinkedListElement != NULL) {
			localPathUtilsContext = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
			*pathUtilsContext = localPathUtilsContext;
		} else {
			result = ENOMEM;
		}
	}

	if (result == SUCCESS) {
		if (!verifyUserAddress || processIsValidSegmentAccess(process, (uint32_t) path, sizeof(char))) {
			result = pathUtilsParsePath(localPathUtilsContext, path, isPathNormalized, includeLastPathSegment,
					process->currentWorkingDirectory, process->currentWorkingDirectoryLength);
		} else {
			result = EFAULT;
		}
	}

	return result;
}

static void releasePathUtilsContext(struct PathUtilsContext** pathUtilsContext) {
	if (*pathUtilsContext != NULL) {
		struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) *pathUtilsContext);
		memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
		*pathUtilsContext = NULL;
	}
}

static APIStatusCode retrieveChild(struct VirtualFileSystemNode* parentVirtualFileSystemNode, struct Process* process,
		const char* childName, struct VirtualFileSystemNode** childVirtualFileSystemNode) {
	APIStatusCode result = SUCCESS;

	struct VirtualFileSystemOperations* operations = parentVirtualFileSystemNode->operations;
	if (operations->walk != NULL) {
		result = operations->walk(parentVirtualFileSystemNode, process, childName, false, 0, childVirtualFileSystemNode, NULL);
		if (result == ENOENT) {
			result = SUCCESS;
		}

	} else {
		result = EPERM;
	}

	return result;
}

static APIStatusCode calculateNewOffset(off_t fileSize, off_t currentOffset, off_t offset, int whence, bool canExceedSize, off_t* newOffset) {
	int64_t localNewOffset;

	switch (whence) {
		case SEEK_SET:
			localNewOffset = (int64_t) offset;
		break;

		case SEEK_CUR:
			localNewOffset = (int64_t) currentOffset + (int64_t) offset;
		break;

		case SEEK_END:
			localNewOffset = (int64_t) fileSize + (int64_t) offset;
		break;

		default:
			localNewOffset = 0;
			assert(false);
		break;
	}

	APIStatusCode result;
	if (localNewOffset > FILE_MAX_SIZE) {
		result = EOVERFLOW;

	} else if (!(0 <= localNewOffset && (canExceedSize || localNewOffset <= fileSize))) {
		result = EINVAL;

	} else {
		*newOffset = (off_t) localNewOffset;
		result = SUCCESS;
	}

	return result;
}

APIStatusCode ioServicesFindLowestAvailableFileDescriptorIndex(struct Process* process, int minimumFileDescriptorIndex, int* fileDescriptorIndex) {
	if (0 <= minimumFileDescriptorIndex && minimumFileDescriptorIndex < MAX_FILE_DESCRIPTORS_PER_PROCESS) {
		int localFileDescriptorIndex = minimumFileDescriptorIndex;

		while (localFileDescriptorIndex < MAX_FILE_DESCRIPTORS_PER_PROCESS
				&& process->fileDescriptors[localFileDescriptorIndex].openFileDescription != NULL) {
			localFileDescriptorIndex++;
		}

		if (localFileDescriptorIndex >= MAX_FILE_DESCRIPTORS_PER_PROCESS) {
			return EMFILE;
		} else {
			*fileDescriptorIndex = localFileDescriptorIndex;
			return SUCCESS;
		}

	} else {
		return EINVAL;
	}
}

APIStatusCode ioServicesOpen(struct Process* process, bool verifyUserAddress, const char* path, bool isPathNormalized, int flags, mode_t mode, int* fileDescriptorIndex) {
	mode = (mode & ~process->fileModeCreationMask) & 0777;

	APIStatusCode result = ioServicesFindLowestAvailableFileDescriptorIndex(process, 0, fileDescriptorIndex);

	struct DoubleLinkedListElement* doubleLinkedListElement = NULL;

	if (result == SUCCESS) {
		struct PathUtilsContext* pathUtilsContext = NULL;
		result = parsePath(process, verifyUserAddress, path, true, isPathNormalized, &pathUtilsContext);
		if (result == SUCCESS) {
			struct VirtualFileSystemNode* virtualFileSystemNode;
			bool followLastSymbolicLink = (O_DO_NOT_FOLLOW_LAST_SYMBOLIC_LINK & flags) == 0;
			bool createIfDoesNotExist = (O_CREAT & flags) != 0;
			bool failIfAlreadyExists = createIfDoesNotExist && (O_EXCL & flags) != 0;
			result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, followLastSymbolicLink, createIfDoesNotExist, mode, failIfAlreadyExists,
					&virtualFileSystemNode);

			if (result == SUCCESS) {
				assert(virtualFileSystemNode != NULL);
				assert(virtualFileSystemNode->usageCount > 0);

				struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;
				struct OpenFileDescription* openFileDescription = virtualFileSystemManagerAcquireOpenFileDescription();
				if (openFileDescription == NULL) {
					result = EMFILE;

				} else {
					openFileDescription->flags = flags;

					if (operations->open != NULL) {
						assert(operations->getMode != NULL);
						mode_t mode = operations->getMode(virtualFileSystemNode);

						if ((flags & O_DIRECTORY) && !S_ISDIR(mode)) {
							result = ENOTDIR;
						} else {
							result = operations->open(virtualFileSystemNode, process, &openFileDescription, flags);
							if (result == SUCCESS && (flags & O_TRUNC) != 0 && operations->changeFileSize != NULL) {
								result = operations->changeFileSize(virtualFileSystemNode, process, openFileDescription, 0);
							}
						}

					} else {
						result = EPERM;
					}

					if (result != SUCCESS) {
						virtualFileSystemManagerReleaseOpenFileDescription(openFileDescription);
					} else {
						openFileDescription->virtualFileSystemNode = virtualFileSystemNode;
						openFileDescription->usageCount++;

						struct FileDescriptor* fileDescriptor = &process->fileDescriptors[*fileDescriptorIndex];
						fileDescriptor->openFileDescription = openFileDescription;
						fileDescriptor->flags = (flags & O_CLOEXEC) ? FD_CLOEXEC: 0;
					}
				}

				if (result != SUCCESS) {
					virtualFileSystemManagerReleaseNodeReservation(process, virtualFileSystemNode, NULL);
				}
			}
		}
		releasePathUtilsContext(&pathUtilsContext);
	}

	if (doubleLinkedListElement != NULL) {
		memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
	}

	return result;
}

APIStatusCode ioServicesReleaseDirectory(struct Process* process, const char* directoryPathToRelease, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;

	struct PathUtilsContext* pathUtilsContext = NULL;
	result = parsePath(process, verifyUserAddress, directoryPathToRelease, false, false, &pathUtilsContext);
	if (result == SUCCESS) {
		if (pathUtilsContext->hasLastSegment) {
			struct VirtualFileSystemNode* parentVirtualFileSystemNode;
			result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, true, false, 0, false, &parentVirtualFileSystemNode);
			if (result == SUCCESS) {
				assert(parentVirtualFileSystemNode != NULL);
				assert(parentVirtualFileSystemNode->usageCount > 0);

				struct VirtualFileSystemOperations* operations = parentVirtualFileSystemNode->operations;
				assert(operations->getMode != NULL);

				if (operations->releaseDirectory == NULL) {
					result = EPERM;

				} else if (!S_ISDIR(operations->getMode(parentVirtualFileSystemNode))) {
					result = ENOTDIR;

				} else {
					struct VirtualFileSystemNode* childVirtualFileSystemNode;
					result = retrieveChild(parentVirtualFileSystemNode, process, pathUtilsContext->lastSegment, &childVirtualFileSystemNode);
					if (result == SUCCESS) {
						if (childVirtualFileSystemNode != NULL) {
							if (S_ISDIR(operations->getMode(childVirtualFileSystemNode))) {
								if (childVirtualFileSystemNode->usageCount == 1) {
									result = operations->releaseDirectory(parentVirtualFileSystemNode, process, childVirtualFileSystemNode);
								} else {
									result = EBUSY;
								}

							} else {
								result = ENOTDIR;
							}

							if (result != SUCCESS) {
								virtualFileSystemManagerReleaseNodeReservation(process, childVirtualFileSystemNode, NULL);
							}

						} else {
							result = ENOENT;
						}
					}
				}

				virtualFileSystemManagerReleaseNodeReservation(process, parentVirtualFileSystemNode, NULL);
			}

		} else {
			result = EINVAL;
		}
	}
	releasePathUtilsContext(&pathUtilsContext);

	return result;
}

APIStatusCode ioServicesReleaseName(struct Process* process, const char* namePathToRelease, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;

	struct PathUtilsContext* pathUtilsContext = NULL;
	result = parsePath(process, verifyUserAddress, namePathToRelease, false, false, &pathUtilsContext);
	if (result == SUCCESS) {
		if (pathUtilsContext->hasLastSegment) {
			assert(pathUtilsContext->segmentCount >= 1);

			struct VirtualFileSystemNode* virtualFileSystemNode;
			result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, true, false, 0, false, &virtualFileSystemNode);
			if (result == SUCCESS) {
				struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;
				assert(operations->getMode != NULL);

				if (operations->releaseName == NULL) {
					result = EPERM;

				} else if (!S_ISDIR(operations->getMode(virtualFileSystemNode))) {
					result = ENOTDIR;

				} else {
					result = operations->releaseName(virtualFileSystemNode, process, pathUtilsContext->lastSegment);
				}

				virtualFileSystemManagerReleaseNodeReservation(process, virtualFileSystemNode, NULL);
			}

		} else {
			result = EINVAL;
		}
	}
	releasePathUtilsContext(&pathUtilsContext);

	return result;
}

APIStatusCode ioServicesCreateDirectory(struct Process* process, const char* newDirectoryPath, bool verifyUserAddress, mode_t mode) {
	APIStatusCode result = SUCCESS;

	mode = (mode & ~process->fileModeCreationMask) & 0777;

	struct PathUtilsContext* pathUtilsContext = NULL;
	result = parsePath(process, verifyUserAddress, newDirectoryPath, false, false, &pathUtilsContext);
	if (result == SUCCESS) {
		if (pathUtilsContext->hasLastSegment) {
			struct VirtualFileSystemNode* virtualFileSystemNode;
			result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, true, false, 0, false, &virtualFileSystemNode);
			if (result == SUCCESS) {
				assert(virtualFileSystemNode != NULL);
				assert(virtualFileSystemNode->usageCount > 0);

				struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;
				assert(operations->getMode != NULL);

				if (operations->createDirectory == NULL) {
					result = EPERM;

				} else if (!S_ISDIR(operations->getMode(virtualFileSystemNode))) {
					result = ENOTDIR;

				} else {
					struct VirtualFileSystemNode* childVirtualFileSystemNode;
					result = retrieveChild(virtualFileSystemNode, process, pathUtilsContext->lastSegment, &childVirtualFileSystemNode);
					if (result == SUCCESS) {
						if (childVirtualFileSystemNode != NULL) {
							virtualFileSystemManagerReleaseNodeReservation(process, childVirtualFileSystemNode, NULL);
							result = EEXIST;
						} else {
							result = operations->createDirectory(virtualFileSystemNode, process, pathUtilsContext->lastSegment, mode);
						}
					}
				}

				virtualFileSystemManagerReleaseNodeReservation(process, virtualFileSystemNode, NULL);
			}

		} else {
			result = EINVAL;
		}
	}
	releasePathUtilsContext(&pathUtilsContext);

	return result;
}

APIStatusCode ioServicesClose(struct Process* process, int fileDescriptorIndex) {
	struct OpenFileDescription* openFileDescription;

	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &openFileDescription);
	if (result == SUCCESS) {
		result = virtualFileSystemManagerCloseOpenFileDescription(process, openFileDescription);
		if (result == SUCCESS) {
			process->fileDescriptors[fileDescriptorIndex].openFileDescription = NULL;
		}
	}

	return result;
}

APIStatusCode ioServicesRead(struct Process* currentProcess, int fileDescriptorIndex, bool verifyUserAddress, void* buffer, size_t bufferSize, size_t* count) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, currentProcess, &openFileDescription);
	if (result == SUCCESS) {
		if ((openFileDescription->flags & O_RDONLY) != 0 || (openFileDescription->flags & O_RDWR) != 0) {
			struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
			struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

			if (verifyUserAddress && !processIsValidSegmentAccess(currentProcess, (uint32_t) buffer, bufferSize)) {
				result = EFAULT;

			} else if (operations->read == NULL) {
				result = EPERM;

			} else {
				result = operations->read(virtualFileSystemNode, currentProcess, openFileDescription, buffer, bufferSize, count);
			}

		} else {
			result = EINVAL;
		}
	}

	return result;
}

static APIStatusCode calculateStatusCodeIfRepositionNotAllowed(struct VirtualFileSystemNode* virtualFileSystemNode) {
	APIStatusCode result;
	struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;
	assert(operations->getMode != NULL);

	mode_t mode = operations->getMode(virtualFileSystemNode);
	if (S_ISFIFO(mode) || S_ISSOCK(mode)) {
		result = ESPIPE;
	} else {
		result = EPERM;
	}

	return result;
}

APIStatusCode ioServicesRepositionOpenFileDescriptionOffset(struct Process* process, int fileDescriptorIndex, off_t offset, int whence, off_t* newOffset) {
	APIStatusCode result;
	if (whence == SEEK_CUR || whence == SEEK_END || whence == SEEK_SET) {
		struct OpenFileDescription* openFileDescription;
		result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &openFileDescription);
		if (result == SUCCESS) {
			struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
			struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

			if (operations->getOpenFileDescriptionOffsetRepositionPolicy != NULL) {
				enum OpenFileDescriptionOffsetRepositionPolicy openFileDescriptionOffsetRepositionPolicy = operations->getOpenFileDescriptionOffsetRepositionPolicy(virtualFileSystemNode);
				switch (openFileDescriptionOffsetRepositionPolicy) {
					case REPOSITION_NOT_ALLOWED:
						result = calculateStatusCodeIfRepositionNotAllowed(virtualFileSystemNode);
						break;

					case ALWAYS_REPOSITION_TO_ZERO:
						openFileDescription->offset = 0;
						*newOffset = 0;
						break;

					case REPOSITION_FREELY:
					case REPOSITION_BASED_ON_FILE_SIZE: {
						assert(operations->getSize != NULL);
						int64_t fileSize = operations->getSize(virtualFileSystemNode);

						result = calculateNewOffset(fileSize, openFileDescription->offset, offset, whence, openFileDescriptionOffsetRepositionPolicy == REPOSITION_FREELY, newOffset);
						if (result == SUCCESS) {
							openFileDescription->offset = *newOffset;
						}
					} break;

					default:
						assert(false);
						result = calculateStatusCodeIfRepositionNotAllowed(virtualFileSystemNode);
						break;
				}

			} else {
				result = calculateStatusCodeIfRepositionNotAllowed(virtualFileSystemNode);
			}
		}

	} else {
		result = EINVAL;
	}


	return result;
}

APIStatusCode ioServicesWrite(struct Process* process, int fileDescriptorIndex, bool verifyUserAddress, void* buffer, size_t bufferSize, size_t* count) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &openFileDescription);
	if (result == SUCCESS) {
		if ((openFileDescription->flags & O_WRONLY) != 0 || (openFileDescription->flags & O_RDWR) != 0) {
			struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
			struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

			if (verifyUserAddress && !processIsValidSegmentAccess(process, (uint32_t) buffer, bufferSize)) {
				result = EFAULT;

			} else if (operations->write == NULL) {
				result = EPERM;

			} else {
				bool restoreOffset = false;
				off_t offset = openFileDescription->offset;
				if ((openFileDescription->flags & O_APPEND) != 0) {
					if (operations->getSize != NULL) {
						off_t size = operations->getSize(virtualFileSystemNode);
						openFileDescription->offset = size;
						restoreOffset = true;
					}
				}
				result = operations->write(virtualFileSystemNode, process, openFileDescription, buffer, bufferSize, count);
				if (restoreOffset) {
					openFileDescription->offset = offset;
				}
			}

		} else {
			result = EINVAL;
		}
	}
	return result;
}

APIStatusCode ioServicesStatus(struct Process* process, int fileDescriptorIndex, bool verifyUserAddress, struct stat* statInstance) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &openFileDescription);
	if (result == SUCCESS) {
		struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
		struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

		if (verifyUserAddress && !processIsValidSegmentAccess(process, (uint32_t) statInstance, sizeof(stat))) {
			result = EFAULT;

		} else if (operations->status == NULL) {
			result = EPERM;

		} else {
			assert(operations->getMode != NULL);
			memset(statInstance, 0, sizeof(struct stat));
			statInstance->st_mode = operations->getMode(virtualFileSystemNode);
			result = operations->status(virtualFileSystemNode, process, openFileDescription, statInstance);
		}
	}

	return result;
}

APIStatusCode ioServicesCreateName(struct Process* process, const char* targetPath, const char* namePathToCreate, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;

	struct PathUtilsContext* pathUtilsContext = NULL;

	struct VirtualFileSystemNode* targetVirtualFileSystemNode = NULL;
	result = parsePath(process, verifyUserAddress, targetPath, true, false, &pathUtilsContext);
	if (result == SUCCESS) {
		result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, true, false, 0, false, &targetVirtualFileSystemNode);
		if (result == SUCCESS && S_ISDIR(targetVirtualFileSystemNode->operations->getMode(targetVirtualFileSystemNode))) {
			result = EPERM;
		}
	}

	struct VirtualFileSystemNode* parentVirtualFileSystemNode = NULL;
	if (result == SUCCESS) {
		result = parsePath(process, verifyUserAddress, namePathToCreate, false, false, &pathUtilsContext);
		if (result == SUCCESS) {
			result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, true, false, 0, false, &parentVirtualFileSystemNode);
			if (result == SUCCESS && !S_ISDIR(parentVirtualFileSystemNode->operations->getMode(parentVirtualFileSystemNode))) {
				result = ENOTDIR;
			}
		}
	}

	if (result == SUCCESS) {
		if (pathUtilsContext->hasLastSegment) {
			struct VirtualFileSystemOperations* operations = parentVirtualFileSystemNode->operations;

			if (operations->createName == NULL) {
				result = EPERM;

			} else {
				struct VirtualFileSystemNode* childVirtualFileSystemNode;
				result = retrieveChild(parentVirtualFileSystemNode, process, pathUtilsContext->lastSegment, &childVirtualFileSystemNode);
				if (result == SUCCESS) {
					if (childVirtualFileSystemNode != NULL) {
						virtualFileSystemManagerReleaseNodeReservation(process, childVirtualFileSystemNode, NULL);
						result = EEXIST;
					} else {
						result = operations->createName(parentVirtualFileSystemNode, process, targetVirtualFileSystemNode, pathUtilsContext->lastSegment);
					}
				}
			}

		} else {
			result = EINVAL;
		}
	}

	if (targetVirtualFileSystemNode != NULL) {
		virtualFileSystemManagerReleaseNodeReservation(process, targetVirtualFileSystemNode, NULL);
	}
	if (parentVirtualFileSystemNode != NULL) {
		virtualFileSystemManagerReleaseNodeReservation(process, parentVirtualFileSystemNode, NULL);
	}
	releasePathUtilsContext(&pathUtilsContext);

	return result;
}

APIStatusCode ioServicesChangeFileSize(struct Process* process, int fileDescriptorIndex, off_t newSize) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription((int) fileDescriptorIndex, process, &openFileDescription);
	if (result == SUCCESS) {
		if ((openFileDescription->flags & O_WRONLY) != 0 || (openFileDescription->flags & O_RDWR) != 0) {
			struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
			struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

			if (operations->changeFileSize == NULL) {
				result = EPERM;

			} else {
				result = operations->changeFileSize(virtualFileSystemNode, process, openFileDescription, newSize);
			}

		} else {
			result = EINVAL;
		}
	}

	return result;
}

APIStatusCode ioServicesCreateSymbolicLink(struct Process* process, const char* targetPath, const char* symbolicLinkPathToCreate, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;

	if (!verifyUserAddress || processIsValidSegmentAccess(process, (uint32_t) targetPath, sizeof(char))) {
		size_t targetPathLength = strlen(targetPath);
		if (targetPathLength + 1 <= PATH_MAX_LENGTH) {
			struct PathUtilsContext* pathUtilsContext = NULL;
			result = parsePath(process, verifyUserAddress, symbolicLinkPathToCreate, false, false, &pathUtilsContext);
			if (result == SUCCESS) {
				if (pathUtilsContext->hasLastSegment) {
					struct VirtualFileSystemNode* virtualFileSystemNode;
					result = virtualFileSystemManagerResolvePath(process, pathUtilsContext, true, false, 0, false, &virtualFileSystemNode);
					if (result == SUCCESS) {
						assert(virtualFileSystemNode != NULL);
						assert(virtualFileSystemNode->usageCount > 0);

						struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;
						assert(operations->getMode != NULL);

						if (operations->createSymbolicLink == NULL) {
							result = EPERM;

						} else if (!S_ISDIR(operations->getMode(virtualFileSystemNode))) {
							result = ENOTDIR;

						} else {
							struct VirtualFileSystemNode* childVirtualFileSystemNode;
							result = retrieveChild(virtualFileSystemNode, process, pathUtilsContext->lastSegment, &childVirtualFileSystemNode);
							if (result == SUCCESS) {
								if (childVirtualFileSystemNode != NULL) {
									virtualFileSystemManagerReleaseNodeReservation(process, childVirtualFileSystemNode, NULL);
									result = EEXIST;
								} else {
									result = operations->createSymbolicLink(virtualFileSystemNode, process, pathUtilsContext->lastSegment, targetPath, targetPathLength);
								}
							}
						}

						virtualFileSystemManagerReleaseNodeReservation(process, virtualFileSystemNode, NULL);
					}

				} else {
					result = EINVAL;
				}
			}

			releasePathUtilsContext(&pathUtilsContext);

		} else {
			result = ENAMETOOLONG;
		}

	} else {
		result = EFAULT;
	}

	return result;
}

APIStatusCode ioServicesChangeDeviceParameters(struct Process* currentProcess, int fileDescriptorIndex, uint32_t* request, bool verifyUserAddress) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, currentProcess, &openFileDescription);
	if (result == SUCCESS) {
		struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
		struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

		if (verifyUserAddress && !processIsValidSegmentAccess(currentProcess, (uint32_t) request, sizeof(uint32_t))) {
			result = EFAULT;

		} else if (operations->manipulateDeviceParameters == NULL) {
			result = EPERM;

		} else {
			result = operations->manipulateDeviceParameters(virtualFileSystemNode, currentProcess, openFileDescription, request);
		}
	}
	return result;
}

APIStatusCode ioServicesChangeOpenFileDescriptionParameters(struct Process* process, int fileDescriptorIndex, uint32_t* command, bool verifyUserAddress) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &openFileDescription);
	if (result == SUCCESS) {
		struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
		struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

		if (verifyUserAddress && !processIsValidSegmentAccess(process, (uint32_t) command, sizeof(uint32_t))) {
			result = EFAULT;

		} else {
			switch (*command) {
				case F_GETFL: {
					int** argument = ((void*) command) + sizeof(void*);
					if (processIsValidSegmentAccess(process, (uint32_t) argument, sizeof(void*))
							&& processIsValidSegmentAccess(process, (uint32_t) *argument, sizeof(int))) {
						**argument = openFileDescription->flags;
					} else {
						result = EFAULT;
					}
				} break;

				case F_SETFL: {
					int* argument = ((void*) command) + sizeof(void*);

					if (processIsValidSegmentAccess(process, (uint32_t) argument, sizeof(void*))) {
						int newFlags = ~(O_APPEND | O_NONBLOCK) & openFileDescription->flags;
						openFileDescription->flags = newFlags | (*argument & (O_APPEND | O_NONBLOCK));
					} else {
						result = EFAULT;
					}
				} break;

				default:
					if (operations->manipulateOpenFileDescriptionParameters == NULL) {
						result = EPERM;
					} else {
						result = operations->manipulateOpenFileDescriptionParameters(virtualFileSystemNode, process, openFileDescription, command);
					}
					break;
			}
		}
	}
	return result;
}

APIStatusCode ioServicesChangeFileDescriptorParameters(struct Process* process, int fileDescriptorIndex, uint32_t* command, bool verifyUserAddress) {
	struct OpenFileDescription* unused;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &unused);
	if (result == SUCCESS) {
		struct FileDescriptor* fileDescriptor = &process->fileDescriptors[fileDescriptorIndex];

		if (verifyUserAddress && !processIsValidSegmentAccess(process, (uint32_t) command, sizeof(uint32_t))) {
			result = EFAULT;

		} else {
			switch (*command) {
				case F_GETFD: {
					int** argument = ((void*) command) + sizeof(void*);
					if (processIsValidSegmentAccess(process, (uint32_t) argument, sizeof(void*))
							&& processIsValidSegmentAccess(process, (uint32_t) *argument, sizeof(int))) {
						**argument = fileDescriptor->flags;
					} else {
						result = EFAULT;
					}
				} break;

				case F_SETFD: {
					int* argument = ((void*) command) + sizeof(void*);
					if (processIsValidSegmentAccess(process, (uint32_t) argument, sizeof(void*))) {
						int newFlags = ~FD_CLOEXEC & fileDescriptor->flags;
						fileDescriptor->flags = newFlags | (*argument & FD_CLOEXEC);
					} else {
						result = EFAULT;
					}
				} break;

				default:
					result = EINVAL;
					break;
			}
		}
	}

	return result;
}

APIStatusCode ioServicesSetCurrentWorkingDirectory(struct Process* process, char* newCurrentWorkingDirectory) {
	APIStatusCode result = SUCCESS;

	struct PathUtilsContext* pathUtilsContext = NULL;
	result = parsePath(process, true, newCurrentWorkingDirectory, true, false, &pathUtilsContext);
	if (result == SUCCESS) {
		int fileDescriptorIndex;
		result = ioServicesOpen(process, false, pathUtilsContext->buffer, true, O_PATH | O_DIRECTORY, 0, &fileDescriptorIndex);
		if (result == SUCCESS) {
			ioServicesClose(process, fileDescriptorIndex);

			size_t newCurrentWorkingDirectoryLength = strlen(pathUtilsContext->buffer);
			assert(newCurrentWorkingDirectoryLength + 1 <= PATH_MAX_LENGTH);
			strcpy(process->currentWorkingDirectory, pathUtilsContext->buffer);
			process->currentWorkingDirectoryLength = newCurrentWorkingDirectoryLength;
		}
	}
	releasePathUtilsContext(&pathUtilsContext);

	return result;
}


APIStatusCode ioServicesReadDirectoryEntry(struct Process* process, int fileDescriptorIndex, struct dirent* direntInstance, bool verifyUserAddress, bool* endOfDirectory) {
	struct OpenFileDescription* openFileDescription;
	APIStatusCode result = virtualFileSystemManagerValidateAndGetOpenFileDescription(fileDescriptorIndex, process, &openFileDescription);
	if (result == SUCCESS) {
		struct VirtualFileSystemNode* virtualFileSystemNode = openFileDescription->virtualFileSystemNode;
		struct VirtualFileSystemOperations* operations = virtualFileSystemNode->operations;

		if (verifyUserAddress && !processIsValidSegmentAccess(process, (uint32_t) direntInstance, sizeof(struct dirent))) {
			result = EFAULT;

		} else if (operations->readDirectoryEntry == NULL) {
			result = EPERM;

		} else {
			result = operations->readDirectoryEntry(virtualFileSystemNode, process, openFileDescription, direntInstance, endOfDirectory);
		}
	}
	return result;
}

APIStatusCode ioServicesDuplicateFileDescriptor(struct Process* process, int existentFileDescriptorIndex,
		int minimumFileDescriptorIndex, int* newFileDescriptorIndex, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;
	if (!verifyUserAddress || processIsValidSegmentAccess(process, (uint32_t) newFileDescriptorIndex, sizeof(int))) {
		struct OpenFileDescription* openFileDescription;
		result = virtualFileSystemManagerValidateAndGetOpenFileDescription(existentFileDescriptorIndex, process, &openFileDescription);
		if (result == SUCCESS) {
			if (*newFileDescriptorIndex < 0 || existentFileDescriptorIndex != *newFileDescriptorIndex) {
				/* It will select the lowest one possible according to the informed value. */
				if (*newFileDescriptorIndex < 0) {
					result = ioServicesFindLowestAvailableFileDescriptorIndex(process, minimumFileDescriptorIndex, newFileDescriptorIndex);
				} else {
					if (*newFileDescriptorIndex >= MAX_FILE_DESCRIPTORS_PER_PROCESS) {
						result = EBADF;
					} else {
						ioServicesClose(process, *newFileDescriptorIndex);
					}
				}

				if (result == SUCCESS) {
					struct FileDescriptor* fileDescriptor = &process->fileDescriptors[*newFileDescriptorIndex];
					assert(fileDescriptor->openFileDescription == NULL);
					fileDescriptor->openFileDescription = openFileDescription;
					fileDescriptor->flags = 0;
					openFileDescription->usageCount++;
				}
			}
		}

	} else {
		result = EFAULT;
	}
	return result;
}

static void resumeProcessExecutionAfterWaitingIOEvent(struct Process* process) {
	assert(processManagerGetProcessById(process->id) != NULL);
	assert(process->ioEventMonitoringCommandSchedulerId != NULL);

	/* Need to check if it is waiting. It could have been stopped. */
	if (process->state == SUSPENDED_WAITING_IO_EVENT) {
		processManagerChangeProcessState(processManagerGetCurrentProcess(), process, RUNNABLE, 0);
	}

	process->ioEventMonitoringCommandSchedulerId = NULL;
}

APIStatusCode ioServicesMonitorIOEvents(struct Process* currentProcess, struct pollfd* userIOEventMonitoringContexts,
		nfds_t ioEventMonitoringContextCount, int timeout, int* triggeredEventsCount) {
	assert(currentProcess->usedIOEventMonitoringContextsCount == 0);
	assert(currentProcess->ioEventMonitoringCommandSchedulerId == NULL);

	APIStatusCode result = SUCCESS;

	if (ioEventMonitoringContextCount > MAX_FILE_DESCRIPTORS_PER_PROCESS) {
		result = EINVAL;

	} else if (!processIsValidSegmentAccess(currentProcess, (uint32_t) userIOEventMonitoringContexts, sizeof(struct pollfd) * ioEventMonitoringContextCount)) {
		result = EFAULT;

	} else {
		struct IOEventMonitoringContext* ioEventMonitoringContexts = currentProcess->ioEventMonitoringContexts;
		int nonNegativeFileDescriptorIndexesCount = 0;
		int i;
		for (i = 0; i < ioEventMonitoringContextCount; i++) {
			struct pollfd* userIOEventMonitoringContext = &userIOEventMonitoringContexts[i];
			userIOEventMonitoringContext->revents = 0;

			struct IOEventMonitoringContext* ioEventMonitoringContext = &ioEventMonitoringContexts[i];
			ioEventMonitoringContext->isBeingMonitored = false;
			ioEventMonitoringContext->process = currentProcess;
			ioEventMonitoringContext->openFileDescription = NULL;
			memcpy(&ioEventMonitoringContext->userIoEventMonitoringContext, &userIOEventMonitoringContexts[i], sizeof(struct pollfd));

			if (userIOEventMonitoringContext->fd >= 0) {
				struct OpenFileDescription* openFileDescription;
				result = virtualFileSystemManagerValidateAndGetOpenFileDescription(userIOEventMonitoringContext->fd, currentProcess, &openFileDescription);
				if (result == SUCCESS) {
					struct VirtualFileSystemOperations* operations = openFileDescription->virtualFileSystemNode->operations;
					if (operations->startIoEventMonitoring != NULL) {
						assert(operations->stopIoEventMonitoring != NULL);
						ioEventMonitoringContext->openFileDescription = openFileDescription;
						nonNegativeFileDescriptorIndexesCount++;

					} else {
						result = EPERM;
						break;
					}

				} else {
					break;
				}
			}
		}

		currentProcess->usedIOEventMonitoringContextsCount = i;

		*triggeredEventsCount = 0;

		if (result == SUCCESS && nonNegativeFileDescriptorIndexesCount > 0) {
			bool done = false;
			do {
				*triggeredEventsCount = 0;

				struct IOEventMonitoringContext* ioEventMonitoringContexts = currentProcess->ioEventMonitoringContexts;
				for (int i = 0; i < currentProcess->usedIOEventMonitoringContextsCount; i++) {
					struct IOEventMonitoringContext* ioEventMonitoringContext = &ioEventMonitoringContexts[i];
					ioEventMonitoringContext->isBeingMonitored = false;

					if (ioEventMonitoringContext->userIoEventMonitoringContext.fd >= 0) {
						struct OpenFileDescription* openFileDescription = ioEventMonitoringContext->openFileDescription;
						assert(openFileDescription != NULL);
						struct VirtualFileSystemOperations* operations = openFileDescription->virtualFileSystemNode->operations;
						assert(operations->startIoEventMonitoring != NULL && operations->stopIoEventMonitoring != NULL);
						result = operations->startIoEventMonitoring(openFileDescription->virtualFileSystemNode, currentProcess, openFileDescription, ioEventMonitoringContext);
						if (result != SUCCESS) {
							done = true;
							break;

						} else if (ioEventMonitoringContext->userIoEventMonitoringContext.revents != 0) {
							(*triggeredEventsCount)++;
							short revents = ioEventMonitoringContext->userIoEventMonitoringContext.revents;
							struct pollfd* userIOEventMonitoringContext = &userIOEventMonitoringContexts[i];
							userIOEventMonitoringContext->revents = revents;
							done = true;

						} else {
							ioEventMonitoringContext->isBeingMonitored = true;
						}
					}
				}

				if (result == SUCCESS) {
					if (timeout == 0) {
						done = true;

					} else if (!done) {
						if (timeout > 0) {
							currentProcess->ioEventMonitoringCommandSchedulerId = commandSchedulerSchedule(timeout, false, (void (*)(void*)) &resumeProcessExecutionAfterWaitingIOEvent, currentProcess);
							if (currentProcess->ioEventMonitoringCommandSchedulerId == NULL) {
								result = ENOMEM;
								done = true;
							}
						}

						if (!done) {
							processServicesSuspendToWaitForIO(currentProcess, NULL, SUSPENDED_WAITING_IO_EVENT);

							enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
							assert(currentProcess->waitingIOProcessList == NULL);
							assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

							if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
								result = EINTR;
								done = true;

							} else {
								/* Check if any desired event has been triggered. */
								*triggeredEventsCount = 0;
								for (int i = 0; i < currentProcess->usedIOEventMonitoringContextsCount; i++) {
									struct IOEventMonitoringContext* ioEventMonitoringContext = &currentProcess->ioEventMonitoringContexts[i];
									struct pollfd* userIOEventMonitoringContext = &userIOEventMonitoringContexts[i];
									if (ioEventMonitoringContext->userIoEventMonitoringContext.fd >= 0) {
										short revents = ioEventMonitoringContext->userIoEventMonitoringContext.revents;
										userIOEventMonitoringContext->revents = revents;
										if (revents != 0) {
											(*triggeredEventsCount)++;
										}
									}
								}

								if (*triggeredEventsCount > 0) {
									done = true;

								} else if (timeout > 0) {
									if (currentProcess->ioEventMonitoringCommandSchedulerId != NULL) {
										timeout = commandSchedulerCancel(currentProcess->ioEventMonitoringCommandSchedulerId);
										currentProcess->ioEventMonitoringCommandSchedulerId = NULL;

										if (timeout == 0) {
											/* No time left. */
											done = true;
										}

									} else {
										/* Timeout! */
										done = true;
									}

								} else {
									assert(timeout <= 0);
									done = timeout == 0;
								}
							}
						}
					}
				}

				if (currentProcess->ioEventMonitoringCommandSchedulerId != NULL) {
					commandSchedulerCancel(currentProcess->ioEventMonitoringCommandSchedulerId);
					currentProcess->ioEventMonitoringCommandSchedulerId = NULL;
				}

			} while (!done);
		}
	}

	/* Release all resource acquired previously. */
	processStopMonitoringIOEvents(currentProcess);

	currentProcess->usedIOEventMonitoringContextsCount = 0;

	return result;
}

static APIStatusCode willBecomeSubdirectoryOfItself(struct Process* currentProcess,
		struct VirtualFileSystemNode* targetVirtualFileSystemNode, struct VirtualFileSystemNode* parentOfToBeReplacedVirtualFileSystemNode, bool* answer) {
	APIStatusCode result = SUCCESS;
	*answer = false;

	struct VirtualFileSystemNode* child = parentOfToBeReplacedVirtualFileSystemNode;
	struct VirtualFileSystemNode* parent = NULL;

	while (result == SUCCESS) {
		if (targetVirtualFileSystemNode == child) {
			/*
			 * If the new path has as one of its members the node that will be renamed, it will be a situation where a directory will become a subdirectory of itself.
			 * Note that as the directory hierarchy is organized as a tree, each directory has a single parent and a single path to the root.
			 * Therefore, it is enough to check if the new path has the node that is being renamed.
			 */
			*answer = true;
			break;

		} else {
			struct VirtualFileSystemOperations* operations = child->operations;
			if (operations->getDirectoryParent != NULL) {
				result = operations->getDirectoryParent(child, currentProcess, &parent);
				if (result == SUCCESS) {
					if (parent == NULL) {
						break;

					} else {
						if (child != parentOfToBeReplacedVirtualFileSystemNode) {
							virtualFileSystemManagerReleaseNodeReservation(currentProcess, child, NULL);
						}
						child = parent;
					}
				}

			} else {
				result = EPERM;
			}
		}
	}

	if (child != parentOfToBeReplacedVirtualFileSystemNode) {
		virtualFileSystemManagerReleaseNodeReservation(currentProcess, child, NULL);
	}


	return result;
}

APIStatusCode ioServicesRename(struct Process* currentProcess, const char* oldPath, const char* newPath, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;

	struct PathUtilsContext* pathUtilsContext = NULL;

	char oldName[FILE_NAME_MAX_LENGTH];
	struct VirtualFileSystemNode* parentOfTargetVirtualFileSystemNode = NULL;
	struct VirtualFileSystemNode* targetVirtualFileSystemNode = NULL;
	if (result == SUCCESS) {
		result = parsePath(currentProcess, verifyUserAddress, oldPath, false, false, &pathUtilsContext);
	}
	if (result == SUCCESS) {
		if (pathUtilsContext->hasLastSegment) {
			strcpy(oldName, pathUtilsContext->lastSegment);
			result = virtualFileSystemManagerResolvePath(currentProcess, pathUtilsContext, true, false, 0, false,
				&parentOfTargetVirtualFileSystemNode);
		} else {
			result = EINVAL;
		}
	}
	if (result == SUCCESS) {
		result = parsePath(currentProcess, verifyUserAddress, oldPath, true, false, &pathUtilsContext);
	}
	if (result == SUCCESS) {
		result = virtualFileSystemManagerResolvePath(currentProcess, pathUtilsContext, false, false, 0, false,
			&targetVirtualFileSystemNode);
	}

	char newName[FILE_NAME_MAX_LENGTH];
	struct VirtualFileSystemNode* parentOfToBeReplacedVirtualFileSystemNode = NULL;
	struct VirtualFileSystemNode* toBeReplacedVirtualFileSystemNode = NULL;
	if (result == SUCCESS) {
		result = parsePath(currentProcess, verifyUserAddress, newPath, false, false, &pathUtilsContext);
	}
	if (result == SUCCESS) {
		if (pathUtilsContext->hasLastSegment) {
			strcpy(newName, pathUtilsContext->lastSegment);
			result = virtualFileSystemManagerResolvePath(currentProcess, pathUtilsContext, true, false, 0, false,
				&parentOfToBeReplacedVirtualFileSystemNode);
		} else {
			result = EINVAL;
		}
	}
	if (result == SUCCESS) {
		result = parsePath(currentProcess, verifyUserAddress, newPath, true, false, &pathUtilsContext);
	}
	if (result == SUCCESS) {
		result = virtualFileSystemManagerResolvePath(currentProcess, pathUtilsContext, false, false, 0, false,
			&toBeReplacedVirtualFileSystemNode);
		if (result == ENOENT) {
			result = SUCCESS;
			toBeReplacedVirtualFileSystemNode = NULL;
		}
	}

	if (result == SUCCESS) {
		assert(parentOfTargetVirtualFileSystemNode != NULL);
		assert(targetVirtualFileSystemNode != NULL);
		assert(parentOfToBeReplacedVirtualFileSystemNode != NULL);

		struct FileSystem* fileSystem = NULL;
		struct VirtualFileSystemOperations* operations;

		operations = parentOfTargetVirtualFileSystemNode->operations;
		assert(operations->getMode != NULL);
		mode_t parentOfTargetNodeMode = operations->getMode(parentOfTargetVirtualFileSystemNode);
		if (operations->getFileSystem != NULL) {
			fileSystem = operations->getFileSystem(parentOfTargetVirtualFileSystemNode);
		} else {
			result = EPERM;
		}

		operations = targetVirtualFileSystemNode->operations;
		assert(operations->getMode != NULL);
		mode_t targetNodeMode = operations->getMode(targetVirtualFileSystemNode);
		if (operations->getFileSystem != NULL) {
			if (fileSystem != operations->getFileSystem(targetVirtualFileSystemNode)) {
				result = EXDEV;
			}
		} else {
			result = EPERM;
		}

		operations = parentOfToBeReplacedVirtualFileSystemNode->operations;
		assert(operations->getMode != NULL);
		mode_t parentOfToBeReplacedNodeMode = operations->getMode(parentOfToBeReplacedVirtualFileSystemNode);
		if (operations->getFileSystem != NULL) {
			if (fileSystem != operations->getFileSystem(parentOfToBeReplacedVirtualFileSystemNode)) {
				result = EXDEV;
			}
		} else {
			result = EPERM;
		}

		mode_t toBeReplacedNodeMode;
		if (toBeReplacedVirtualFileSystemNode != NULL) {
			operations = toBeReplacedVirtualFileSystemNode->operations;
			assert(operations->getMode != NULL);
			toBeReplacedNodeMode = operations->getMode(toBeReplacedVirtualFileSystemNode);
			if (operations->getFileSystem != NULL) {
				if (fileSystem != operations->getFileSystem(toBeReplacedVirtualFileSystemNode)) {
					result = EXDEV;
				}
			} else {
				result = EPERM;
			}

		} else {
			toBeReplacedNodeMode = 0;
		}

		if (result == SUCCESS && toBeReplacedVirtualFileSystemNode != NULL && S_ISDIR(toBeReplacedNodeMode)) {
			assert(toBeReplacedVirtualFileSystemNode->usageCount >= 1);
			if (toBeReplacedVirtualFileSystemNode->usageCount > 1) {
				result = EBUSY;
			}
		}

		if (result == SUCCESS) {
			if (S_ISDIR(parentOfTargetNodeMode) && S_ISDIR(parentOfToBeReplacedNodeMode) ) {
				if (S_ISDIR(targetNodeMode)) {
					bool answer;
					result = willBecomeSubdirectoryOfItself(currentProcess, targetVirtualFileSystemNode, parentOfToBeReplacedVirtualFileSystemNode, &answer);
					if (result == SUCCESS && answer) {
						result = EINVAL;
					}
				}

			} else {
				result = EINVAL;
			}
		}

		if (result == SUCCESS) {
			operations = targetVirtualFileSystemNode->operations;
			if (operations->rename != NULL) {
				if (targetVirtualFileSystemNode != toBeReplacedVirtualFileSystemNode) {
					if (toBeReplacedVirtualFileSystemNode == NULL) {
						/* There will be no replacement. */

						result = operations->rename(targetVirtualFileSystemNode, currentProcess, parentOfTargetVirtualFileSystemNode, oldName,
								parentOfToBeReplacedVirtualFileSystemNode, toBeReplacedVirtualFileSystemNode, newName);

					} else {
						if (S_ISDIR(targetNodeMode) != S_ISDIR(toBeReplacedNodeMode)) {
							result = EISDIR;

						} else {
							/* There will be a replacement. Both are folders or both are not folders. */

							result = operations->rename(targetVirtualFileSystemNode, currentProcess, parentOfTargetVirtualFileSystemNode, oldName,
									parentOfToBeReplacedVirtualFileSystemNode, toBeReplacedVirtualFileSystemNode, newName);
						}
					}

				} else {
					/* Nothing to do here! */
				}

			} else {
				result = EPERM;
			}
		}
	}

	if (parentOfTargetVirtualFileSystemNode != NULL) {
		virtualFileSystemManagerReleaseNodeReservation(currentProcess, parentOfTargetVirtualFileSystemNode, NULL);
	}
	if (targetVirtualFileSystemNode != NULL) {
		virtualFileSystemManagerReleaseNodeReservation(currentProcess, targetVirtualFileSystemNode, NULL);
	}
	if (parentOfToBeReplacedVirtualFileSystemNode != NULL) {
		virtualFileSystemManagerReleaseNodeReservation(currentProcess, parentOfToBeReplacedVirtualFileSystemNode, NULL);
	}
	if (toBeReplacedVirtualFileSystemNode != NULL) {
		virtualFileSystemManagerReleaseNodeReservation(currentProcess, toBeReplacedVirtualFileSystemNode, NULL);
	}
	if (pathUtilsContext != NULL) {
		releasePathUtilsContext(&pathUtilsContext);
	}

	return result;
}
