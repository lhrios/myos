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

#ifndef KERNEL_PROCESS_SERVICES_H
	#define KERNEL_PROCESS_SERVICES_H

	#include <stdbool.h>
	#include <stdint.h>

	#include "kernel/api_status_code.h"
	#include "kernel/process/process.h"

	#include "util/double_linked_list.h"

	void processServicesSuspendToWaitForIO(struct Process* currentProcess, struct DoubleLinkedList* list, enum ProcessState newState);
	APIStatusCode processServicesExecuteExecutable(struct Process* process, bool verifyUserAddress, const char* executablePath, const char** argv, const char** envp);
	APIStatusCode processServicesCreateSessionAndProcessGroup(struct Process* leaderProcess);
	APIStatusCode processGetSessionId(struct Process* currentProcess, pid_t processId, pid_t* sessionId);
	APIStatusCode processServicesGetProcessGroupId(struct Process* currentProcess, pid_t processId, pid_t* processGroupId);
	APIStatusCode processServicesWait(struct Process* currentProcess, pid_t scope, int options, int* status, pid_t* childProcessId);
	void processServicesNotifyParentAboutChildStateChange(struct Process* currentProcess, struct Process* process,
			enum ProcessState currentState, enum ProcessState newState, int sourceSignalId);
	void processServicesSleep(struct Process* currentProcess, int seconds);
	APIStatusCode processServicesSetProcessGroup(struct Process* currentProcess, pid_t processId, pid_t processGroupId);
	void processServicesWakeUpProcesses(struct Process* currentProcess, struct DoubleLinkedList* processList, enum ProcessState processState);

#endif
