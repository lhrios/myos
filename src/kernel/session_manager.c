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

#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/session_manager.h"

#include "util/fixed_capacity_sorted_array.h"
#include "util/string_stream_writer.h"

static struct FixedCapacitySortedArray allSessionsArray;
static struct DoubleLinkedList availableSessionsList;

static const pid_t* sessionIdExtractor(const struct Session** session) {
	return &(*session)->id;
}

static int sessionIdComparator(const pid_t* sessionId1, const pid_t* sessionId2) {
	return *sessionId1 - *sessionId2;
}

void sessionManagerReleaseSession(struct Session* session) {
	assert(session != NULL);
	assert(session->usageCount > 0);
	session->usageCount--;

	if (session->usageCount == 0) {
		bool result = fixedCapacitySortedArrayRemove(&allSessionsArray, &session->id);
		assert(result);
		doubleLinkedListInsertBeforeFirst(&availableSessionsList, &session->doubleLinkedListElement);
	}
}

void sessionManagerRemoveProcessGroup(struct ProcessGroup* processGroup) {
	if (processGroup->session != NULL) {
		sessionManagerReleaseSession(processGroup->session);
		processGroup->session = NULL;
	}
}

void sessionManagerInsertProcessGroup(struct Session* session, struct ProcessGroup* processGroup) {
	assert(session != NULL);
	if (processGroup->session != NULL) {
		sessionManagerReleaseSession(processGroup->session);
	}

	session->usageCount++;
	processGroup->session = session;
}

bool sessionManagerHasResourcesForNewOne(void) {
	return doubleLinkedListSize(&availableSessionsList) > 0;
}

struct Session* sessionManagerAcquireSession(struct Process* leaderProcess) {
	assert(fixedCapacitySortedArraySearch(&allSessionsArray, &leaderProcess->id) == NULL);
	assert(leaderProcess->processGroup == NULL);

	if (doubleLinkedListSize(&availableSessionsList) > 0) {
		struct Session* session = (void*) doubleLinkedListRemoveLast(&availableSessionsList);
		memset(session, 0, sizeof(struct Session));
		session->id = leaderProcess->id;
		session->controllingTTYId = -1;
		session->usageCount = 1;
		session->leaderProcess = leaderProcess;

		bool result = fixedCapacitySortedArrayInsert(&allSessionsArray, &session);
		assert(result);

		return session;

	} else {
		return NULL;
	}
}

APIStatusCode sessionManagerInitialize(void) {
	APIStatusCode result = SUCCESS;

	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement == NULL) {
		result = ENOMEM;

	} else {
		fixedCapacitySortedArrayInitialize(&allSessionsArray, sizeof(struct Session*),
				(void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement),
				PAGE_FRAME_SIZE, 	(int (*)(const void*, const void*)) &sessionIdComparator,
				(const void* (*)(const void*)) &sessionIdExtractor);
		doubleLinkedListInitialize(&availableSessionsList);

		doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
		if (doubleLinkedListElement == NULL) {
			result = ENOMEM;
		} else {
			struct Session* sessions = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
			for (int i = 0; i < PAGE_FRAME_SIZE / sizeof(struct Session); i++) {
				struct Session* session = &sessions[i];
				doubleLinkedListInsertAfterLast(&availableSessionsList, &session->doubleLinkedListElement);
			}
		}
	}

	return result;
}

APIStatusCode sessionManagerPrintDebugReport(void) {
	int bufferSize = 512;
	char buffer[bufferSize];
	struct StringStreamWriter stringStreamWriter;

	logDebug("Session manager report:\n");

	for (int i = 0; i < fixedCapacitySortedArraySize(&allSessionsArray); i++) {
		struct Session* session = *((struct Session**) fixedCapacitySortedArrayGet(&allSessionsArray, i));

		stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);
		streamWriterFormat(&stringStreamWriter.streamWriter, "Session %d\n", session->id);
		streamWriterFormat(&stringStreamWriter.streamWriter, "  usageCount: %d\n", session->usageCount);
		streamWriterFormat(&stringStreamWriter.streamWriter, "  controllingTTYId: %d\n", session->controllingTTYId);
		stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

		logDebug("%s", buffer);
	}

	return SUCCESS;
}
