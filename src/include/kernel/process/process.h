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

#ifndef KERNEL_PROCESS_H
	#define KERNEL_PROCESS_H

	#include <limits.h>
	#include <signal.h>
	#include <sys/types.h>

	#include <myos.h>

	#include "kernel/api_status_code.h"
	#include "kernel/memory_manager_constants.h"
	#include "kernel/limits.h"
	#include "kernel/session.h"
	#include "kernel/x86.h"

	#include "kernel/io/file_descriptor.h"

	#include "kernel/process/process_group.h"
	#include "kernel/process/process_execution_state.h"

	#include "util/double_linked_list.h"

	#define USER_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX 4
	#define USER_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX 5

	#define EXECUTABLE_MAX_SIZE (1024 * PAGE_FRAME_SIZE) /* Up to 4 MB */

	#define CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS 0x40000000
	#define DATA_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS (CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS + EXECUTABLE_MAX_SIZE)
	#define STACK_SEGMENT_FIRST_INVALID_VIRTUAL_ADDRESS_AFTER 0xFFFFFFFC /* As the stack grows downward, the address refers to its top when it is empty. */
	#define STACK_PAGE_FRAME_COUNT (16 + ARG_MAX / PAGE_FRAME_SIZE)
	_Static_assert(ARG_MAX % PAGE_FRAME_SIZE == 0, "Expecting ARG_MAX as multiple of PAGE_FRAME_SIZE.");

	#define INIT_PROCESS_ID 1

	enum ProcessState {
		ABSENT = 0,
		RUNNABLE = 1,
		SUSPENDED_SLEEPING = 2,
		SUSPENDED_WAITING_CHILD = 3,
		SUSPENDED_WAITING_READ = 4,
		SUSPENDED_WAITING_WRITE = 5,
		SUSPENDED_WAITING_IO_EVENT = 6,
		STOPPED = 7,
		WAITING_EXIT_STATUS_COLLECTION = 8 /* Also known as a "zombie" or "defunct" process. */
	};

	struct SignalInformation {
		void (*callback)(int, struct sigaction*);
		struct sigaction signalHandlingConfiguration;
		siginfo_t signalCreationInformation;
		bool pending;
		bool ignored;
	};

	struct Process;
	struct IOEventMonitoringContext {
		struct DoubleLinkedListElement doubleLinkedListElement;
		struct Process* process;
		struct OpenFileDescription* openFileDescription;
		struct pollfd userIoEventMonitoringContext;
		bool isBeingMonitored;
	};

	struct ProcessGroup;
	struct Session;
	struct Process {
		struct DoubleLinkedListElement runnableProcessListElement;
		struct DoubleLinkedListElement childrenProcessListElement;
		struct DoubleLinkedListElement waitingIOProcessListElement;

		/*
		 * If this process is suspended because it is waiting some I/O operation to complete,
		 * the following variable contains the list it is waiting on.
		 */
		struct DoubleLinkedList* waitingIOProcessList;

		struct DoubleLinkedList childrenProcessList;

		struct DoubleLinkedList pagingPageFramesList;

		struct DoubleLinkedList codeSegmentPageFramesList;
		struct DoubleLinkedList dataSegmentPageFramesList;
		struct DoubleLinkedList stackSegmentPageFramesList;

		struct X86TaskState x86TaskState;
		uint64_t tssSegmentDescriptor;

		struct ProcessExecutionState1* processExecutionState1;
		struct ProcessExecutionState2* processExecutionState2;
		enum ProcessState state;
		bool executedExecutableAfterFork;

		/*
		 * If it is sleeping, the ID associated with the scheduler command.
		 */
		void* sleepCommandSchedulerId;

		pid_t id;

		/* Scheduler related: */
		volatile uint32_t ticksCountSinceSchedulerIterationBegin;
		volatile uint32_t idOfLastKnownSchedulerIteration;

		void* systemStack;

		struct FileDescriptor fileDescriptors[MAX_FILE_DESCRIPTORS_PER_PROCESS];

		/* Poll related: */
		struct IOEventMonitoringContext ioEventMonitoringContexts[MAX_FILE_DESCRIPTORS_PER_PROCESS];
		int usedIOEventMonitoringContextsCount;
		void* ioEventMonitoringCommandSchedulerId;

		struct Process* parentProcess;

		struct ProcessGroup* processGroup;
		struct DoubleLinkedListElement processGroupListElement;

		int exitStatus;
		int signalToNotifyProcessAbout;
		enum ProcessState stateToNotifyParentAbout;

		/* Signal related: */
		bool mightHaveAnySignalToHandle;
		sigset_t blockedSignalsSet;
		struct SignalInformation signalInformation[NUMBER_OF_SIGNALS];

		char* currentWorkingDirectory;
		size_t currentWorkingDirectoryLength;

		mode_t fileModeCreationMask;

		/* FPU related: */
		bool fpuInitialized;
		uint8_t fpuState[X86_FPU_STATE_LENGTH] __attribute__((aligned (16)));

	};
	_Static_assert(sizeof(struct Process) <= PAGE_FRAME_SIZE,
		"Expecting that a Process instance fits on a page frame block.");

	_Static_assert(offsetof(struct Process, fpuState) % 16 == 0,
		"Expecting a \"fpuState\" field aligned on a 16-byte boundary.");

	bool processIsProcessGroupLeader(struct Process* process);
	bool processIsSessionLeader(struct Process* process);
	void processStopMonitoringIOEvents(struct Process* process);
	void processRemoveFromWaitingIOProcessList(struct Process* process);
	int processCountIOEventsBeingMonitored(struct Process* process);
	struct Session* processGetSession(struct Process* process);
	APIStatusCode processGetCurrentWorkingDirectory(struct Process* currentProcess, char* buffer, size_t bufferSize);
	bool processIsValidSegmentAccess(struct Process* process, uint32_t firstAddress, size_t count);
	APIStatusCode processGetProcessMemorySegmentsLimits(struct Process* process,
			struct ProcessMemorySegmentsLimits* processMemorySegmentsLimits, bool verifyUserAddress);

	struct Process* processGetProcessFromProcessGroupListElement(struct DoubleLinkedListElement* listElement);
	struct Process* processGetProcessFromChildrenProcessListElement(struct DoubleLinkedListElement* listElement);
	struct Process* processGetProcessFromIOProcessListElement(struct DoubleLinkedListElement* listElement);
	struct Process* processGetProcessFromRunnableProcessListElement(struct DoubleLinkedListElement* listElement);

#endif
