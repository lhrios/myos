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

#ifndef KERNEL_PROCESS_MANAGER_H
	#define KERNEL_PROCESS_MANAGER_H

	#include <stdint.h>

	#include "kernel/api_status_code.h"
	#include "kernel/process/process.h"
	#include "kernel/x86.h"

	#include "util/fixed_capacity_sorted_array.h"

	APIStatusCode processManagerInitialize(void);
	struct Process* processManagerGetProcessById(pid_t id);

	enum ResumedProcessExecutionSituation {
		WILL_CALL_SIGNAL_HANDLER,
		NORMAL_EXECUTION_RESUMED
	};
	enum ResumedProcessExecutionSituation processManagerScheduleProcessExecution(void);

	void processManagerStop(struct Process* currentProcess, int signalId);
	void processManagerTerminate(struct Process* currentProcess, int exitStatus, int sourceSignalId);
	APIStatusCode processManagerCreateInitProcess(__attribute__ ((cdecl)) void (*initializationCallback)(void*), void* argument);
	APIStatusCode processManagerForkProcess(struct Process* parentProcess, struct Process** childProcess);
	void processManagerStartScheduling(void);
	__attribute__ ((cdecl)) struct Process* processManagerGetCurrentProcess(void);
	void processManagerChangeProcessState(struct Process* currentProcess, struct Process* targetProcess, enum ProcessState state, int sourceSignalId);
	void processManagerReleaseProcessResources(struct Process* process);
	struct Process* processGetProcessFromChildrenProcessListElement(struct DoubleLinkedListElement* listElement);
	struct Process* processGetProcessFromIOProcessListElement(struct DoubleLinkedListElement* listElement);
	APIStatusCode processManagerChangeCodeSegmentSize(struct Process* process, size_t executableSize, bool allowShrinkingIfNecessary);
	APIStatusCode processManagerChangeDataSegmentSize(struct Process* process, int increment);
	APIStatusCode processManagerPrintDebugReport(void);
	void processManagerInitializeAllProcessesIterator(struct FixedCapacitySortedArrayIterator* fixedCapacitySortedArrayIterator);

#endif
