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

#include "kernel/process/process.h"

bool processIsProcessGroupLeader(struct Process* process) {
	struct ProcessGroup* processGroup = process->processGroup;
	return processGroup != NULL && processGroup->leaderProcess == process;
}

bool processIsSessionLeader(struct Process* process) {
	struct Session* session = processGetSession(process);
	return session->leaderProcess == process;
}

void processStopMonitoringIOEvents(struct Process* process) {
	for (int i = 0; i < process->usedIOEventMonitoringContextsCount; i++) {
		struct IOEventMonitoringContext* ioEventMonitoringContext = &process->ioEventMonitoringContexts[i];
		if (ioEventMonitoringContext->isBeingMonitored) {
			assert(ioEventMonitoringContext->process == process);
			struct OpenFileDescription* openFileDescription = ioEventMonitoringContext->openFileDescription;
			assert(openFileDescription != NULL);
			struct VirtualFileSystemOperations* operations = openFileDescription->virtualFileSystemNode->operations;
			assert(operations->startIoEventMonitoring != NULL && operations->stopIoEventMonitoring != NULL);
			operations->stopIoEventMonitoring(openFileDescription->virtualFileSystemNode, process, openFileDescription, ioEventMonitoringContext);
			ioEventMonitoringContext->isBeingMonitored = false;
		}
	}
}

void processRemoveFromWaitingIOProcessList(struct Process* process) {
	/* If there is some register related with a suspension due to an I/O operation, it will be undone. */
	if (process->waitingIOProcessList != NULL) {
		assert(doubleLinkedListContainsFoward(process->waitingIOProcessList, &process->waitingIOProcessListElement));
		doubleLinkedListRemove(process->waitingIOProcessList, &process->waitingIOProcessListElement);
		process->waitingIOProcessList = NULL;
	}
}

int processCountIOEventsBeingMonitored(struct Process* process) {
	int result = 0;
	for (int i = 0; i < process->usedIOEventMonitoringContextsCount; i++) {
		struct IOEventMonitoringContext* ioEventMonitoringContext = &process->ioEventMonitoringContexts[i];
		if (ioEventMonitoringContext->isBeingMonitored) {
			result++;
		}
	}
	return result;
}

struct Session* processGetSession(struct Process* process) {
	assert(process->processGroup != NULL);
	assert(process->processGroup->session != NULL);
	return process->processGroup->session;
}

APIStatusCode processGetCurrentWorkingDirectory(struct Process* currentProcess, char* buffer, size_t bufferSize) {
	APIStatusCode result = SUCCESS;

	if (processIsValidSegmentAccess(currentProcess, (uint32_t) buffer, bufferSize)) {
		if (currentProcess->currentWorkingDirectoryLength + 1 <= bufferSize) {
			strcpy(buffer, currentProcess->currentWorkingDirectory);
		} else {
			result = ERANGE;
		}

	} else {
		result = EFAULT;
	}

	return result;
}

static inline __attribute__((always_inline))  bool isValidSegmentAccess(uint32_t firstAddress, uint32_t lastAddress, struct ProcessMemorySegmentLimits* processMemorySegmentLimits) {
	 bool result = !processMemorySegmentLimits->isEmpty && processMemorySegmentLimits->firstAddress <= firstAddress
			&& lastAddress < processMemorySegmentLimits->lastAddress;
	 return result;
}

bool processIsValidSegmentAccess(struct Process* process, uint32_t firstAddress, size_t count) {
	if (count == 0) {
		count = 1;
	}

	uint32_t lastAddress = firstAddress + (count - 1);
	bool overflow = lastAddress < firstAddress;

	struct ProcessMemorySegmentsLimits processMemorySegmentsLimits;
	processGetProcessMemorySegmentsLimits(process, &processMemorySegmentsLimits, false);

	bool result = !overflow && (isValidSegmentAccess(firstAddress, lastAddress, &processMemorySegmentsLimits.code)
			|| isValidSegmentAccess(firstAddress, lastAddress, &processMemorySegmentsLimits.data)
			|| isValidSegmentAccess(firstAddress, lastAddress, &processMemorySegmentsLimits.stack));

	return result;
}

static void calculateProcessMemorySegmentLimits(struct ProcessMemorySegmentLimits* processMemorySegmentLimits, struct DoubleLinkedList* dataSegmentPageFramesList,
		uint32_t segmentFirstOrLastAddress, bool isFirstAddress) {
	int pageFrameCount = doubleLinkedListSize(dataSegmentPageFramesList);
	if (pageFrameCount > 0) {
		processMemorySegmentLimits->isEmpty = false;
		uint32_t firstAddress;
		uint32_t lastAddress;

		if (isFirstAddress) {
			firstAddress = segmentFirstOrLastAddress;
			lastAddress = segmentFirstOrLastAddress + pageFrameCount * PAGE_FRAME_SIZE;
		} else {
			lastAddress = segmentFirstOrLastAddress;
			firstAddress = segmentFirstOrLastAddress - pageFrameCount * PAGE_FRAME_SIZE;
			if (firstAddress % PAGE_FRAME_SIZE != 0) {
				firstAddress -= firstAddress % PAGE_FRAME_SIZE;
				firstAddress += PAGE_FRAME_SIZE;
			}
		}
		processMemorySegmentLimits->firstAddress = firstAddress;
		processMemorySegmentLimits->lastAddress = lastAddress;

	} else {
		processMemorySegmentLimits->isEmpty = true;
	}
}

APIStatusCode processGetProcessMemorySegmentsLimits(struct Process* process, struct ProcessMemorySegmentsLimits* processMemorySegmentsLimits,
		bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;
	if (!verifyUserAddress || processIsValidSegmentAccess(process, (uint32_t) processMemorySegmentsLimits, sizeof(struct ProcessMemorySegmentsLimits))) {
		calculateProcessMemorySegmentLimits(&processMemorySegmentsLimits->code, &process->codeSegmentPageFramesList, CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS, true);
		calculateProcessMemorySegmentLimits(&processMemorySegmentsLimits->data, &process->dataSegmentPageFramesList, DATA_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS, true);
		calculateProcessMemorySegmentLimits(&processMemorySegmentsLimits->stack, &process->stackSegmentPageFramesList, STACK_SEGMENT_FIRST_INVALID_VIRTUAL_ADDRESS_AFTER, false);

	} else {
		result = EFAULT;
	}

	return result;
}

struct Process* processGetProcessFromProcessGroupListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct Process, processGroupListElement);
		return (struct Process*) address;
	} else {
		return NULL;
	}
}

struct Process* processGetProcessFromChildrenProcessListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct Process, childrenProcessListElement);
		return (struct Process*) address;
	} else {
		return NULL;
	}
}

struct Process* processGetProcessFromIOProcessListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct Process, waitingIOProcessListElement);
		return (struct Process*) address;
	} else {
		return NULL;
	}
}

struct Process* processGetProcessFromRunnableProcessListElement(struct DoubleLinkedListElement* listElement) {
	if (listElement != NULL) {
		uint32_t address = ((uint32_t) listElement) - offsetof(struct Process, runnableProcessListElement);
		return (struct Process*) address;
	} else {
		return NULL;
	}
}
