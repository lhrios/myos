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

#include <fcntl.h>
#include <string.h>

#include "kernel/cmos.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/process/process_manager.h"

#include "kernel/io/pipe_manager.h"

#include "kernel/services/io_services.h"
#include "kernel/services/process_services.h"
#include "kernel/services/signal_services.h"

#include "kernel/io/virtual_file_system_manager.h"
#include "kernel/io/virtual_file_system_node.h"
#include "kernel/io/virtual_file_system_operations.h"

#include "util/double_linked_list.h"
#include "util/math_utils.h"
#include "util/ring_buffer.h"

static int nextPipeId = 1;

static struct VirtualFileSystemOperations pipeVirtualFileSystemOperations;

static struct DoubleLinkedList availablePipeVirtualFileSystemNodesList;

struct PipeVirtualFileSystemNode {
	struct VirtualFileSystemNode virtualFileSystemNode;
	struct DoubleLinkedListElement* bufferPageFrameListElement;
	void* buffer;
	struct RingBuffer ringBuffer;
	struct DoubleLinkedListElement listElement;
	bool releasedReaderOpenFileDescription;
	bool releasedWriterOpenFileDescription;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	int id;
};

static struct PipeVirtualFileSystemNode* getPipeVirtualFileSystemNodeFromListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct PipeVirtualFileSystemNode, listElement);
		return (struct PipeVirtualFileSystemNode*) address;
	} else {
		return NULL;
	}
}

static void afterNodeReservationRelease(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* currentProcess, struct OpenFileDescription* openFileDescription) {
	assert(openFileDescription != NULL);

	struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct DoubleLinkedList* waitingIOProcessList =  &pipeVirtualFileSystemNode->virtualFileSystemNode.waitingIOProcessList;

	if (openFileDescription->flags & O_RDONLY) {
		pipeVirtualFileSystemNode->releasedReaderOpenFileDescription = true;
		processServicesWakeUpProcesses(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_WRITE);

	} else if (openFileDescription->flags & O_WRONLY) {
		pipeVirtualFileSystemNode->releasedWriterOpenFileDescription = true;
		processServicesWakeUpProcesses(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_READ);

	} else {
		assert(false);
	}

	if (pipeVirtualFileSystemNode->releasedReaderOpenFileDescription && pipeVirtualFileSystemNode->releasedWriterOpenFileDescription) {
		assert(pipeVirtualFileSystemNode->virtualFileSystemNode.usageCount == 0);
		memoryManagerReleasePageFrame(pipeVirtualFileSystemNode->bufferPageFrameListElement, - 1);
		doubleLinkedListInsertAfterLast(&availablePipeVirtualFileSystemNodesList, &pipeVirtualFileSystemNode->listElement);
	}
}

static APIStatusCode status(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNode = (void*) virtualFileSystemNode;

	statInstance->st_atime = pipeVirtualFileSystemNode->st_atime;
	statInstance->st_blksize = PIPE_BUF;
	statInstance->st_blocks = 1;
	statInstance->st_ctime = pipeVirtualFileSystemNode->st_ctime;
	statInstance->st_dev = PIPE_ID;
	statInstance->st_ino = pipeVirtualFileSystemNode->id;
	statInstance->st_mtime = pipeVirtualFileSystemNode->st_mtime;
	statInstance->st_size = ringBufferSize(&pipeVirtualFileSystemNode->ringBuffer);

	return SUCCESS;
}

static APIStatusCode read(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* currentProcess,
		struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct RingBuffer* ringBuffer = &pipeVirtualFileSystemNode->ringBuffer;
	struct DoubleLinkedList* waitingIOProcessList =  &pipeVirtualFileSystemNode->virtualFileSystemNode.waitingIOProcessList;

	assert(openFileDescription->flags & O_RDONLY);
	assert(!pipeVirtualFileSystemNode->releasedReaderOpenFileDescription);

	APIStatusCode result = SUCCESS;

	*count = 0;
	bool done = false;
	do {
		if (ringBufferIsEmpty(ringBuffer)) {
			if (pipeVirtualFileSystemNode->releasedWriterOpenFileDescription) {
				done = true;
			}

		} else {
			*count = ringBufferRead(ringBuffer, buffer, bufferSize);
			processServicesWakeUpProcesses(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_WRITE);
			done = true;
		}

		if (!done) {
			processServicesSuspendToWaitForIO(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_READ);

			enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
			assert(!doubleLinkedListContainsFoward(waitingIOProcessList, &currentProcess->waitingIOProcessListElement));
			assert(currentProcess->waitingIOProcessList == NULL);
			assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

			if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
				return EINTR;
			}
		}

	} while (!done);

	assert(!doubleLinkedListContainsFoward(&openFileDescription->virtualFileSystemNode->waitingIOProcessList, &currentProcess->waitingIOProcessListElement));
	assert(currentProcess->waitingIOProcessList == NULL);
	assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

	return result;
}

static APIStatusCode write(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* currentProcess,
		struct OpenFileDescription* openFileDescription, void* buffer, size_t originalBufferSize, size_t* count) {
	struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct RingBuffer* ringBuffer = &pipeVirtualFileSystemNode->ringBuffer;
	struct DoubleLinkedList* waitingIOProcessList =  &pipeVirtualFileSystemNode->virtualFileSystemNode.waitingIOProcessList;

	assert(openFileDescription->flags & O_WRONLY);
	assert(!pipeVirtualFileSystemNode->releasedWriterOpenFileDescription);

	APIStatusCode result = SUCCESS;

	*count = 0;
	bool done = false;
	size_t bufferSize = originalBufferSize;
	do {
		if (pipeVirtualFileSystemNode->releasedReaderOpenFileDescription) {
			signalServicesGenerateSignal(currentProcess, currentProcess->id, SIGPIPE, false, NULL);
			result = EPIPE;
			done = true;

		} else {
			size_t remaining = ringBufferRemaining(ringBuffer);
			if (originalBufferSize <= PIPE_BUF) {
				if (remaining >= originalBufferSize) {
					ringBufferWrite(ringBuffer, buffer, originalBufferSize);
					*count = originalBufferSize;

					processServicesWakeUpProcesses(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_READ);

					done = true;
				}

			} else {
				if (remaining > 0) {
					remaining = mathUtilsMin(remaining, bufferSize);
					ringBufferWrite(ringBuffer, buffer, remaining);
					*count += remaining;
					buffer += remaining;
					bufferSize -= remaining;

					processServicesWakeUpProcesses(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_READ);

					done = bufferSize == 0;
				}
			}
		}


		if (!done) {
			processServicesSuspendToWaitForIO(currentProcess, waitingIOProcessList, SUSPENDED_WAITING_WRITE);

			enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
			assert(!doubleLinkedListContainsFoward(waitingIOProcessList, &currentProcess->waitingIOProcessListElement));
			assert(currentProcess->waitingIOProcessList == NULL);
			assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

			if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
				return EINTR;
			}
		}

	} while (!done);

	assert(!doubleLinkedListContainsFoward(&openFileDescription->virtualFileSystemNode->waitingIOProcessList, &currentProcess->waitingIOProcessListElement));
	assert(currentProcess->waitingIOProcessList == NULL);
	assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

	return result;
}

APIStatusCode pipeManagerCreatePipe(struct Process* currentProcess, int* readFileDescriptorIndex, int* writeFileDescriptorIndex) {
	APIStatusCode result = SUCCESS;
	struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNode = NULL;
	struct OpenFileDescription* readOpenFileDescription = NULL;
	struct OpenFileDescription* writeOpenFileDescription = NULL;

	*readFileDescriptorIndex = -1;
	*writeFileDescriptorIndex = -1;

	if (doubleLinkedListSize(&availablePipeVirtualFileSystemNodesList) > 0) {
		struct DoubleLinkedListElement* listElement = doubleLinkedListRemoveFirst(&availablePipeVirtualFileSystemNodesList);
		pipeVirtualFileSystemNode = getPipeVirtualFileSystemNodeFromListElement(listElement);
		memset(pipeVirtualFileSystemNode, 0, sizeof(struct PipeVirtualFileSystemNode));
		pipeVirtualFileSystemNode->bufferPageFrameListElement = memoryManagerAcquirePageFrame(true, -1);
		pipeVirtualFileSystemNode->virtualFileSystemNode.operations = &pipeVirtualFileSystemOperations;

		if (pipeVirtualFileSystemNode->bufferPageFrameListElement != NULL) {
			pipeVirtualFileSystemNode->buffer = (void*) memoryManagerGetPageFramePhysicalAddress(pipeVirtualFileSystemNode->bufferPageFrameListElement);
			ringBufferInitialize(&pipeVirtualFileSystemNode->ringBuffer, pipeVirtualFileSystemNode->buffer, PIPE_BUF);

			readOpenFileDescription = virtualFileSystemManagerAcquireOpenFileDescription();
			writeOpenFileDescription = virtualFileSystemManagerAcquireOpenFileDescription();

			if (readOpenFileDescription != NULL && writeOpenFileDescription != NULL) {
				readOpenFileDescription->virtualFileSystemNode = &pipeVirtualFileSystemNode->virtualFileSystemNode;
				readOpenFileDescription->flags = O_RDONLY;

				writeOpenFileDescription->virtualFileSystemNode = &pipeVirtualFileSystemNode->virtualFileSystemNode;
				writeOpenFileDescription->flags = O_WRONLY;

				pipeVirtualFileSystemNode->virtualFileSystemNode.usageCount = 2;

				result = ioServicesFindLowestAvailableFileDescriptorIndex(currentProcess, 0, readFileDescriptorIndex);
				if (result == SUCCESS) {
					struct FileDescriptor* fileDescriptor = &currentProcess->fileDescriptors[*readFileDescriptorIndex];
					fileDescriptor->openFileDescription = readOpenFileDescription;
					fileDescriptor->flags = 0;

					result = ioServicesFindLowestAvailableFileDescriptorIndex(currentProcess, 0, writeFileDescriptorIndex);
					if (result == SUCCESS) {
						time_t unixTime = cmosGetUnixTime();
						pipeVirtualFileSystemNode->st_atime = unixTime;
						pipeVirtualFileSystemNode->st_ctime = unixTime;
						pipeVirtualFileSystemNode->st_mtime = unixTime;
						pipeVirtualFileSystemNode->id = nextPipeId++;

						struct FileDescriptor* fileDescriptor = &currentProcess->fileDescriptors[*writeFileDescriptorIndex];
						fileDescriptor->openFileDescription = writeOpenFileDescription;
						fileDescriptor->flags = 0;

						readOpenFileDescription->usageCount++;
						writeOpenFileDescription->usageCount++;
					}
				}

			} else {
				result = EMFILE;
			}

		} else {
			result = ENOMEM;
		}

	} else {
		result = EMFILE;
	}

	if (result != SUCCESS) {
		if (pipeVirtualFileSystemNode != NULL) {
			if (pipeVirtualFileSystemNode->bufferPageFrameListElement != NULL) {
				memoryManagerReleasePageFrame(pipeVirtualFileSystemNode->bufferPageFrameListElement, -1);
			}
			doubleLinkedListInsertAfterLast(&availablePipeVirtualFileSystemNodesList, &pipeVirtualFileSystemNode->listElement);
		}

		if (readOpenFileDescription != NULL) {
			virtualFileSystemManagerReleaseOpenFileDescription(readOpenFileDescription);
			virtualFileSystemManagerReleaseOpenFileDescription(writeOpenFileDescription);
		}

		if (*readFileDescriptorIndex >= 0) {
			currentProcess->fileDescriptors[*readFileDescriptorIndex].openFileDescription = NULL;
		}

		if (*writeFileDescriptorIndex >= 0) {
			currentProcess->fileDescriptors[*writeFileDescriptorIndex].openFileDescription = NULL;
		}
	}

	return result;
}

static mode_t getMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	return S_IFIFO | S_IRUSR | S_IWUSR;
}

APIStatusCode pipeManagerInitialize(void) {
	APIStatusCode result = SUCCESS;

	doubleLinkedListInitialize(&availablePipeVirtualFileSystemNodesList);

	memset(&pipeVirtualFileSystemOperations, 0, sizeof(struct VirtualFileSystemOperations));
	pipeVirtualFileSystemOperations.afterNodeReservationRelease = &afterNodeReservationRelease;
	pipeVirtualFileSystemOperations.read = &read;
	pipeVirtualFileSystemOperations.write = &write;
	pipeVirtualFileSystemOperations.status = &status;
	pipeVirtualFileSystemOperations.getMode = &getMode;

	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement == NULL) {
		result = ENOMEM;

	} else {
		struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNodes = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		for (int i = 0; i < PAGE_FRAME_SIZE / sizeof(struct PipeVirtualFileSystemNode); i++) {
			struct PipeVirtualFileSystemNode* pipeVirtualFileSystemNode = &pipeVirtualFileSystemNodes[i];
			doubleLinkedListInsertAfterLast(&availablePipeVirtualFileSystemNodesList, &pipeVirtualFileSystemNode->listElement);
		}
	}

	return result;
}
