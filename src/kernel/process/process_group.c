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

#include "kernel/tty.h"

#include "kernel/process/process_group.h"
#include "kernel/process/process_group_manager.h"

void processGroupInsertProcess(struct ProcessGroup* processGroup, struct Process* process) {
	processGroupRemoveProcess(process);
	process->processGroup = processGroup;
	doubleLinkedListInsertAfterLast(&processGroup->processesList, &process->processGroupListElement);
	processGroup->usageCount++;
}

void processGroupRemoveProcess(struct Process* process) {
	struct ProcessGroup* processGroup = process->processGroup;

	if (processGroup != NULL) {
		assert(doubleLinkedListContainsFoward(&processGroup->processesList, &process->processGroupListElement));
		doubleLinkedListRemove(&processGroup->processesList, &process->processGroupListElement);
		processGroupManagerReleaseReservation(processGroup);
		process->processGroup = NULL;

		if (doubleLinkedListSize(&processGroup->processesList) == 0) {
			ttyHandleProcessGroupBecameEmpty(processGroup);
		}
	}
}

int processGroupCountWithState(struct ProcessGroup* processGroup, enum ProcessState processState) {
	int count = 0;

	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&processGroup->processesList);
	while (listElement != NULL) {
		struct Process* process = processGetProcessFromProcessGroupListElement(listElement);
		if (process->state == processState) {
			count++;
		}
		listElement = listElement->next;
	}

	return count;
}

bool processGroupIsOrphaned(struct ProcessGroup* processGroup) {
	if (processGroup->id != INIT_PROCESS_ID) {
		struct Session* processGroupSession = processGroup->session;
		assert(processGroupSession != NULL);

		struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&processGroup->processesList);
		while (listElement != NULL) {
			struct Process* process = processGetProcessFromProcessGroupListElement(listElement);
			assert(processGroupSession == processGetSession(process));
			struct Process* parentProcess = process->parentProcess;
			assert(parentProcess != NULL);

			/*
			 * The POSIX.1 definition of an orphaned process group is one in which the parent of
			 * every member is either itself a member of the group or is not a member of the
			 * group’s session. Another way of saying this is that the process group is not
			 * orphaned as long as a process in the group has a parent in a different process
			 * group but in the same session.
			 *
			 * Advanced Programming in the UNIX Environment, third edition
			 */
			if (process->state != WAITING_EXIT_STATUS_COLLECTION && process->processGroup != parentProcess->processGroup &&
					processGroupSession == processGetSession(parentProcess)) {
				return false;
			}
			listElement = listElement->next;
		}

		return true;

	} else {
		return false;
	}
}
