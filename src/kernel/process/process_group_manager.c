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

#include <string.h>

#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/session_manager.h"

#include "kernel/process/process_group_manager.h"

#include "util/fixed_capacity_sorted_array.h"
#include "util/string_stream_writer.h"

static struct FixedCapacitySortedArray allProcessGroupsArray;
static struct DoubleLinkedList availableProcessGroupsList;

static const pid_t* processGroupIdExtractor(const struct ProcessGroup** processGroup) {
	return &(*processGroup)->id;
}

static int processGroupIdComparator(const pid_t* processGroupId1, const pid_t* processGroupId2) {
	return *processGroupId1 - *processGroupId2;
}

struct ProcessGroup* processGroupManagerGetAndReserveProcessGroupById(pid_t id) {
	struct ProcessGroup** processGroup = fixedCapacitySortedArraySearch(&allProcessGroupsArray, &id);
	if (processGroup != NULL) {
		assert((*processGroup)->usageCount > 0);
		(*processGroup)->usageCount++;
		return *processGroup;

	} else {
		return NULL;
	}
}

struct ProcessGroup* processGroupManagerAcquireProcessGroup(struct Process* leaderProcess) {
	assert(fixedCapacitySortedArraySearch(&allProcessGroupsArray, &leaderProcess->id) == NULL);

	if (doubleLinkedListSize(&availableProcessGroupsList) > 0) {
		struct ProcessGroup* processGroup = (void*) doubleLinkedListRemoveLast(&availableProcessGroupsList);
		memset(processGroup, 0, sizeof(struct ProcessGroup));
		doubleLinkedListInitialize(&processGroup->processesList);

		processGroup->id = leaderProcess->id;
		processGroup->leaderProcess = leaderProcess;
		processGroup->usageCount = 1;
		processGroupInsertProcess(processGroup, leaderProcess);

		bool result = fixedCapacitySortedArrayInsert(&allProcessGroupsArray, &processGroup);
		assert(result);
		return processGroup;

	} else {
		return NULL;
	}
}

void processGroupManagerReleaseReservation(struct ProcessGroup* processGroup) {
	assert(processGroup->usageCount > 0);
	processGroup->usageCount--;
	if (processGroup->usageCount == 0) {
		sessionManagerRemoveProcessGroup(processGroup);
		bool result = fixedCapacitySortedArrayRemove(&allProcessGroupsArray, &processGroup->id);
		assert(result);
		doubleLinkedListInsertBeforeFirst(&availableProcessGroupsList, &processGroup->doubleLinkedListElement);
	}
}

APIStatusCode processGroupManagerInitialize(void) {
	APIStatusCode result = SUCCESS;

	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement == NULL) {
		result = ENOMEM;

	} else {
		fixedCapacitySortedArrayInitialize(&allProcessGroupsArray, sizeof(struct ProcessGroup*),
				(void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement),
				PAGE_FRAME_SIZE, 	(int (*)(const void*, const void*)) &processGroupIdComparator,
				(const void* (*)(const void*)) &processGroupIdExtractor);
		doubleLinkedListInitialize(&availableProcessGroupsList);

		doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
		if (doubleLinkedListElement == NULL) {
			result = ENOMEM;
		} else {
			struct ProcessGroup* processGroups = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
			for (int i = 0; i < PAGE_FRAME_SIZE / sizeof(struct ProcessGroup); i++) {
				struct ProcessGroup* processGroup = &processGroups[i];
				doubleLinkedListInsertAfterLast(&availableProcessGroupsList, &processGroup->doubleLinkedListElement);
			}
		}
	}

	return result;
}

APIStatusCode processGroupManagerPrintDebugReport(void) {
	int bufferSize = 512;
	char buffer[bufferSize];
	struct StringStreamWriter stringStreamWriter;

	logDebug("Process group manager report:\n");

	for (int i = 0; i < fixedCapacitySortedArraySize(&allProcessGroupsArray); i++) {
		struct ProcessGroup* processGroup = *((struct ProcessGroup**) fixedCapacitySortedArrayGet(&allProcessGroupsArray, i));

		stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);
		streamWriterFormat(&stringStreamWriter.streamWriter, "ProcessGroup %d\n", processGroup->id);
		streamWriterFormat(&stringStreamWriter.streamWriter, "  usageCount: %d\n", processGroup->usageCount);
		streamWriterFormat(&stringStreamWriter.streamWriter, "  processesList size: %d\n", doubleLinkedListSize(&processGroup->processesList));
		stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

		logDebug("%s", buffer);
	}

	return SUCCESS;
}

bool processGroupManagerHasResourcesForNewOne(void) {
	return doubleLinkedListSize(&availableProcessGroupsList) > 0;
}
