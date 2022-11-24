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
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "kernel/assembly_globals.h"
#include "kernel/command_scheduler.h"
#include "kernel/error_handler.h"
#include "kernel/interruption_manager.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/system_calls.h"
#include "kernel/system_call_manager.h"
#include "kernel/pit.h"
#include "kernel/session_manager.h"
#include "kernel/tty.h"
#include "kernel/x86.h"

#include "kernel/io/open_file_description.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "kernel/process/process.h"
#include "kernel/process/process_group.h"
#include "kernel/process/process_group_manager.h"
#include "kernel/process/process_manager.h"

#include "kernel/services/process_services.h"
#include "kernel/services/signal_services.h"

#include "util/double_linked_list.h"
#include "util/fixed_capacity_sorted_array.h"
#include "util/math_utils.h"
#include "util/string_stream_writer.h"

const char* PROCESS_STATE_NAMES[] = {
	"ABSENT",
	"RUNNABLE",
	"SUSPENDED_SLEEPING",
	"SUSPENDED_WAITING_CHILD",
	"SUSPENDED_WAITING_READ",
	"SUSPENDED_WAITING_WRITE",
	"SUSPENDED_WAITING_IO_EVENT",
	"STOPPED",
	"WAITING_EXIT_STATUS_COLLECTION"
};

#define TICKS_PER_SCHEDULER_ITERATION_PER_PROCESS 3 /* Each tick represents 10 ms approximately. */

extern uint8_t INITIALIZATION_STACK_TOP[];

static struct X86TaskState systemX86TaskState;
static uint32_t __attribute__((aligned(PAGE_FRAME_SIZE))) systemX86TaskPageDirectory[PAGE_DIRECTORY_LENGTH];
static uint64_t systemX86TaskTSSSegmentDescriptor;

static volatile uint32_t schedulerIterationId = 1;
static volatile int nextProcessId = INIT_PROCESS_ID;

static struct FixedCapacitySortedArray allProcessesArray;
static struct DoubleLinkedList runnableProcessesList;

static struct Process* volatile currentProcess = NULL; /* It is a volatile pointer to a non-volatile memory area. */
static struct Process* initProcess = NULL;

static struct FixedCapacitySortedArray possibleOrphanedProcessGroupsArray;

static bool isValidProcessStateChange(enum ProcessState currentState, enum ProcessState newState) {
	switch (currentState) {
		case ABSENT:
			return false;
		case RUNNABLE:
			return newState != ABSENT;
		case SUSPENDED_WAITING_IO_EVENT:
		case SUSPENDED_WAITING_READ:
		case SUSPENDED_WAITING_WRITE:
		case SUSPENDED_WAITING_CHILD:
		case SUSPENDED_SLEEPING:
			return newState == RUNNABLE;
		case STOPPED:
			return newState == RUNNABLE;
		default:
			return false;
	}
}

void processManagerChangeProcessState(struct Process* currentProcess, struct Process* targetProcess, enum ProcessState newState, int sourceSignalId) {
	assert(targetProcess->state != WAITING_EXIT_STATUS_COLLECTION);
	if (!isValidProcessStateChange(targetProcess->state, newState)) {
		logFatal("Invalid process state change for process %d: from %s to %s", targetProcess->id, PROCESS_STATE_NAMES[targetProcess->state], PROCESS_STATE_NAMES[newState]);
		assert(false);
	}

	processServicesNotifyParentAboutChildStateChange(currentProcess, targetProcess, targetProcess->state, newState, sourceSignalId);

	if (newState == RUNNABLE) {
		if (targetProcess->state != RUNNABLE) {
			assert(!doubleLinkedListContainsFoward(&runnableProcessesList, &targetProcess->runnableProcessListElement));
			doubleLinkedListInsertAfterLast(&runnableProcessesList, &targetProcess->runnableProcessListElement);
			targetProcess->state = RUNNABLE;
		}

	} else {
		if (targetProcess->state == RUNNABLE) {
			assert(doubleLinkedListContainsFoward(&runnableProcessesList, &targetProcess->runnableProcessListElement));
			doubleLinkedListRemove(&runnableProcessesList, &targetProcess->runnableProcessListElement);
		}
		targetProcess->state = newState;
	}
}

static void initializeSystemEntriesOfPageDirectory(uint32_t* pageDirectory, uint32_t systemPageTableCount) {
	for (int i = 0; i < PAGE_DIRECTORY_LENGTH; i++) {
		uint32_t pageDirectoryEntry;
		if (i < systemPageTableCount) {
			uint32_t pageTableAddress = (uint32_t) memoryManagerGetSystemPageTableAddress(i);
			assert((pageTableAddress % PAGE_FRAME_SIZE) == 0);
			pageDirectoryEntry = pageTableAddress | PAGE_ENTRY_PRESENT | PAGE_ENTRY_READ_WRITE | PAGE_ENTRY_SYSTEM | PAGE_ENTRY_CACHE_ENABLED |
					PAGE_ENTRY_SIZE_4_KBYTES | PAGE_ENTRY_LOCAL;

		} else {
			pageDirectoryEntry = PAGE_ENTRY_NOT_PRESENT;
		}

		pageDirectory[i] = pageDirectoryEntry;
	}
}

static void initializeX86TaskState(struct X86TaskState* x86TaskState, uint16_t codeSegmentSelector, uint16_t dataSegmentSelector,

		uint32_t pageDirectoryAddress,
		uint32_t esp0,

		uint32_t eip, uint32_t esp, uint32_t eflags) {

	x86TaskState->previousTaskLink = 0;
	x86TaskState->reserved0 = 0;

	x86TaskState->esp0 = esp0;
	assert(x86TaskState->esp0 % 4 == 0);
	x86TaskState->ss0 = x86SegmentSelector(SYSTEM_KERNEL_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX, false, 0);
	x86TaskState->reserved1 = 0;

	x86TaskState->esp1 = 0;
	x86TaskState->ss1 = 0;
	x86TaskState->reserved2 = 0;

	x86TaskState->esp2 = 0;
	x86TaskState->ss2 = 0;
	x86TaskState->reserved3 = 0;

	x86TaskState->cr3 = pageDirectoryAddress;

	x86TaskState->eip = eip;
	x86TaskState->eflags = eflags;
	x86TaskState->eax = 0;
	x86TaskState->ecx = 0;
	x86TaskState->edx = 0;
	x86TaskState->ebx = 0;
	x86TaskState->esp = esp;
	assert(x86TaskState->esp % 4 == 0);
	x86TaskState->ebp = 0;
	x86TaskState->esi = 0;
	x86TaskState->edi = 0;

	x86TaskState->es = dataSegmentSelector;
	x86TaskState->reserved4 = 0;
	x86TaskState->cs = codeSegmentSelector;
	x86TaskState->reserved5 = 0;
	x86TaskState->ss = dataSegmentSelector;
	x86TaskState->reserved6 = 0;
	x86TaskState->ds = dataSegmentSelector;
	x86TaskState->reserved7 = 0;
	x86TaskState->fs = dataSegmentSelector;
	x86TaskState->reserved8 = 0;
	x86TaskState->gs = dataSegmentSelector;
	x86TaskState->reserved9 = 0;

	x86TaskState->ldt = 0;
	x86TaskState->reserved10 = 0;

	x86TaskState->debugTrapAndReserved11 = 0;
	x86TaskState->ioMapBaseAddress = sizeof(struct X86TaskState); /* It is not allowed to use the I/O ports. */
}

static void releasePageFrameList(struct DoubleLinkedList* doubleLinkedList) {
	struct DoubleLinkedListElement* pageFrame = doubleLinkedListFirst(doubleLinkedList);
	while (pageFrame != NULL) {
		struct DoubleLinkedListElement* nextPageFrame = pageFrame->next;
		memoryManagerReleasePageFrame(pageFrame, -1);
		pageFrame = nextPageFrame;
	}
}

static void closeAllFileDescriptors(struct Process*  process) {
	for (int i = 0; i < MAX_FILE_DESCRIPTORS_PER_PROCESS; i++) {
		struct FileDescriptor* fileDescriptor = &process->fileDescriptors[i];
		virtualFileSystemManagerCloseOpenFileDescription(process, fileDescriptor->openFileDescription);
		process->fileDescriptors[i].openFileDescription = NULL;
	}
}

void processManagerReleaseProcessResources(struct Process* process) {
	assert(process != NULL);

	logDebug("Freeing resources of process %d", process->id);

	if (process->currentWorkingDirectory != NULL) {
		memoryManagerReleasePageFrame(memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) process->currentWorkingDirectory), -1);
	}

	if (process->sleepCommandSchedulerId != NULL) {
		commandSchedulerCancel(process->sleepCommandSchedulerId);
		process->sleepCommandSchedulerId = NULL;
	}

	if (process->ioEventMonitoringCommandSchedulerId != NULL) {
		commandSchedulerCancel(process->ioEventMonitoringCommandSchedulerId);
		process->ioEventMonitoringCommandSchedulerId = NULL;
	}

	if (process->parentProcess != NULL) {
		assert(doubleLinkedListContainsFoward(&process->parentProcess->childrenProcessList, &process->childrenProcessListElement));
		doubleLinkedListRemove(&process->parentProcess->childrenProcessList, &process->childrenProcessListElement);
	}

	fixedCapacitySortedArrayRemove(&allProcessesArray, &process->id);

	releasePageFrameList(&process->codeSegmentPageFramesList);
	releasePageFrameList(&process->stackSegmentPageFramesList);
	releasePageFrameList(&process->dataSegmentPageFramesList);

	releasePageFrameList(&process->pagingPageFramesList);

	processGroupRemoveProcess(process);

	closeAllFileDescriptors(process);

	if (process->systemStack != NULL) {
		memoryManagerReleasePageFrame(memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) process->systemStack), -1);
	}
	memoryManagerReleasePageFrame(memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) process), -1);
}

void processManagerStop(struct Process* currentProcess, int signalId) {
	assert(currentProcess->state != WAITING_EXIT_STATUS_COLLECTION);
	assert(currentProcess != initProcess);
	assert(signalId == SIGSTOP || signalId == SIGTSTP || signalId == SIGTTIN || signalId == SIGTTOU);

	if (currentProcess->state != STOPPED) {
		processRemoveFromWaitingIOProcessList(currentProcess);
		processStopMonitoringIOEvents(currentProcess);

		processManagerChangeProcessState(currentProcess, currentProcess, STOPPED, signalId);
	}
}

void processManagerTerminate(struct Process* currentProcess, int exitStatus, int sourceSignalId) {
	if (currentProcess == initProcess) {
		errorHandlerFatalError("Init process cannot terminate");
	}

	assert(currentProcess->state == RUNNABLE);
	assert(currentProcess->parentProcess != NULL);

	/* There is not pending here as we already released the resources no matter what happened. */
	assert(currentProcess->waitingIOProcessList == NULL);
	assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

	currentProcess->exitStatus = exitStatus;

	struct Session* currentProcessSession = processGetSession(currentProcess);

	fixedCapacitySortedArrayClear(&possibleOrphanedProcessGroupsArray);

	if (currentProcessSession == processGetSession(currentProcess->parentProcess)) {
		/* After the termination of this process, its process group can become orphaned. */
		fixedCapacitySortedArrayInsert(&possibleOrphanedProcessGroupsArray, &currentProcess->processGroup);
	}

	/* This process has any child? */
	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&currentProcess->childrenProcessList);
	while (listElement) {
		struct Process* childProcess = processGetProcessFromChildrenProcessListElement(listElement);
		assert(childProcess->parentProcess == currentProcess);
		/* It needs to be done now, before changing the parent, as it will use the same DoubleLinkedListElement. */
		listElement = listElement->next;


		struct Session* childProcessSession = processGetSession(childProcess);
		if (childProcessSession == currentProcessSession) {
			/* After the termination of this process, the child's process group can become orphaned. */
			fixedCapacitySortedArrayInsert(&possibleOrphanedProcessGroupsArray, &childProcess->processGroup);
		}

		/* Now, it has a new parent. */
		childProcess->parentProcess = initProcess;
		doubleLinkedListInsertAfterLast(&initProcess->childrenProcessList, &childProcess->childrenProcessListElement);
	}

	processManagerChangeProcessState(currentProcess, currentProcess, WAITING_EXIT_STATUS_COLLECTION, sourceSignalId);

	closeAllFileDescriptors(currentProcess);

	/* Is it a session leader that is controlling a TTY? */
	if (currentProcessSession->leaderProcess == currentProcess && currentProcessSession->controllingTTYId != -1) {
		/* We release the TTY as soon as possible. */
		ttyHandleSessionLeaderTermination(currentProcessSession->controllingTTYId, currentProcess);
	}

	/*
	 * If the exit of the process causes a process group to become orphaned, and if any member of the
    * newly-orphaned process group is stopped, then a SIGHUP signal followed by a SIGCONT signal shall
    * be sent to each process in the newly-orphaned process group. (https://pubs.opengroup.org/onlinepubs/9699919799/functions/_exit.html)
	 */
	for (int i = 0; i < fixedCapacitySortedArraySize(&possibleOrphanedProcessGroupsArray); i++) {
		struct ProcessGroup* processGroup = *((struct ProcessGroup**) fixedCapacitySortedArrayGet(&possibleOrphanedProcessGroupsArray, i));

		if (processGroupIsOrphaned(processGroup) && processGroupCountWithState(processGroup, STOPPED)) {
			if (signalServicesGenerateSignal(currentProcess, -processGroup->id, SIGHUP, false, NULL) == SUCCESS) {
				signalServicesGenerateSignal(currentProcess, -processGroup->id, SIGCONT, false, NULL);
			}
		}
	}
}

static void doX86TaskSwitch(struct Process* newCurrentProcess) {
	currentProcess = newCurrentProcess;

	if (newCurrentProcess == NULL) {
		x86WriteOnGDT(TASK_STATE_SEGMENT_DESCRIPTOR_INDEX, systemX86TaskTSSSegmentDescriptor);
	} else {
		x86WriteOnGDT(TASK_STATE_SEGMENT_DESCRIPTOR_INDEX, newCurrentProcess->tssSegmentDescriptor);
	}

	uint64_t taskSegmentSelector = ((uint64_t) x86SegmentSelector(TASK_STATE_SEGMENT_DESCRIPTOR_INDEX, false, 0)) << 32;

	/*
	 * When the processor executes a task switch, its current state will be saved on the task that is being executed.
	 * There will be always one because we have started one during kernel initialization. However, the process state
	 * will be stored on kernel stack associated with the current task (before the switch). Therefore, when it schedules the
	 * process again, the kernel execution will be resumed just after the "ljmp" instruction.
	 */
	__asm__ __volatile__("ljmp *%0"
		:
		: "m"(*&taskSegmentSelector)
		: "cc", "memory");
}

/**
 * Implements a round-robin scheduling policy.
 */
static enum ResumedProcessExecutionSituation doScheduleProcessExecution(uint64_t tickCount, uint64_t upTimeInMilliseconds, bool incrementTicksCountSinceSchedulerIterationBegin) {
	bool done = false;
	enum ResumedProcessExecutionSituation resumedProcessExecutionSituation;

	do {
		bool doTaskSwitch = false;
		struct Process* process = NULL; /* only for iterations */
		struct Process* newCurrentProcessCandidate = NULL; /* While the decision has not been concluded yet. */
		struct Process* newCurrentProcess = NULL;

		newCurrentProcessCandidate = NULL;

		if (currentProcess != NULL && currentProcess->state != RUNNABLE) {
			if (doubleLinkedListSize(&runnableProcessesList) > 0) {
				process = processGetProcessFromRunnableProcessListElement(doubleLinkedListFirst(&runnableProcessesList));
			}
		} else {
			process = currentProcess;
		}

		while (process != NULL) {
			if (process->idOfLastKnownSchedulerIteration != schedulerIterationId) {
				assert(process->idOfLastKnownSchedulerIteration < schedulerIterationId);
				process->ticksCountSinceSchedulerIterationBegin = 1;
				process->idOfLastKnownSchedulerIteration = schedulerIterationId;
				newCurrentProcessCandidate = process;
				break;

			} else {
				if (incrementTicksCountSinceSchedulerIterationBegin) {
					if (process->ticksCountSinceSchedulerIterationBegin + 1 <= TICKS_PER_SCHEDULER_ITERATION_PER_PROCESS) {
						process->ticksCountSinceSchedulerIterationBegin++;
						newCurrentProcessCandidate = process;
						break;
					}
				} else {
					newCurrentProcessCandidate = process;
					break;
				}
			}

			process = processGetProcessFromRunnableProcessListElement(process->runnableProcessListElement.next);
		}

		if (newCurrentProcessCandidate == NULL && doubleLinkedListSize(&runnableProcessesList) > 0) {
			schedulerIterationId++;
			newCurrentProcessCandidate = processGetProcessFromRunnableProcessListElement(doubleLinkedListFirst(&runnableProcessesList));
			newCurrentProcessCandidate->ticksCountSinceSchedulerIterationBegin = 1;
			newCurrentProcessCandidate->idOfLastKnownSchedulerIteration = schedulerIterationId;
		}

		newCurrentProcess = NULL;
		doTaskSwitch = false;
		if (newCurrentProcessCandidate != NULL) {
			assert(newCurrentProcessCandidate->state == RUNNABLE);
			assert(newCurrentProcessCandidate->ticksCountSinceSchedulerIterationBegin <= TICKS_PER_SCHEDULER_ITERATION_PER_PROCESS);

			newCurrentProcess = newCurrentProcessCandidate;
			if (newCurrentProcessCandidate != currentProcess) {
				doTaskSwitch = true;
			}

		} else if (currentProcess != NULL) {
			newCurrentProcess = NULL;
			doTaskSwitch = true;
		}

		if (newCurrentProcessCandidate != NULL) {
			process = processGetProcessFromRunnableProcessListElement(newCurrentProcessCandidate->runnableProcessListElement.next);
		}

		done = true;
		resumedProcessExecutionSituation = NORMAL_EXECUTION_RESUMED;
		if (doTaskSwitch) {
			doX86TaskSwitch(newCurrentProcess);

			if (currentProcess != NULL) {
				assert(currentProcess->state == RUNNABLE);

				processRemoveFromWaitingIOProcessList(currentProcess);
				processStopMonitoringIOEvents(currentProcess);

				while (currentProcess->mightHaveAnySignalToHandle && done) {
					int signalIdToNotifyAbout;
					switch (signalServicesCalculateSignalToHandle(currentProcess, &signalIdToNotifyAbout)) {
						case CONTINUE_PROCESS_EXECUTION_THEN_USER_CALLBACK:
						case USER_CALLBACK:
							signalServicesNotifyProcessAboutSignal(currentProcess, signalIdToNotifyAbout);
							resumedProcessExecutionSituation = WILL_CALL_SIGNAL_HANDLER;
							break;

						case STOP_PROCESS:
						case TERMINATE_PROCESS:
							done = false;
							break;

						case DO_NOTHING:
						case CONTINUE_PROCESS_EXECUTION:
							break;

						default:
							assert(false);
							break;
					}
				}
			}
		}

	} while (!done);

	return resumedProcessExecutionSituation;
}

enum ResumedProcessExecutionSituation processManagerScheduleProcessExecution(void) {
	return doScheduleProcessExecution(0, 0, false);
}

static void scheduleProcessExecutionAfterInterruptionHandler(uint64_t tickCount, uint64_t upTimeInMilliseconds) {
	doScheduleProcessExecution(0, 0, true);
}

__attribute__ ((cdecl)) void processStart();

static struct Process* doCreateProcess(__attribute__ ((cdecl)) void (*initializationCallback)(void*), void* argument) {
	if (fixedCapacitySortedArrayRemaining(&allProcessesArray) > 0) {
		struct DoubleLinkedListElement* processPageFrame = memoryManagerAcquirePageFrame(true, -1);
		if (processPageFrame == NULL) {
			return NULL;
		}
		struct Process* process = (struct Process*) memoryManagerGetPageFramePhysicalAddress(processPageFrame);
		memset(process, 0, sizeof(struct Process));

		doubleLinkedListInitialize(&process->pagingPageFramesList);
		doubleLinkedListInitialize(&process->codeSegmentPageFramesList);
		doubleLinkedListInitialize(&process->dataSegmentPageFramesList);
		doubleLinkedListInitialize(&process->stackSegmentPageFramesList);
		doubleLinkedListInitialize(&process->childrenProcessList);
		process->state = RUNNABLE;
		process->id = nextProcessId++;
		process->idOfLastKnownSchedulerIteration = schedulerIterationId;
		process->ticksCountSinceSchedulerIterationBegin = TICKS_PER_SCHEDULER_ITERATION_PER_PROCESS; /* It will be scheduled only on next iteration. */
		sigemptyset(&process->blockedSignalsSet);

		struct DoubleLinkedListElement* currentWorkingDirectoryPageFrameListElement = memoryManagerAcquirePageFrame(true, -1);
		if (currentWorkingDirectoryPageFrameListElement == NULL) {
			processManagerReleaseProcessResources(process);
			return NULL;
		}
		process->currentWorkingDirectory = (void*) memoryManagerGetPageFramePhysicalAddress(currentWorkingDirectoryPageFrameListElement);

		struct DoubleLinkedListElement* systemStackPageFrame = memoryManagerAcquirePageFrame(true, -1);
		if (systemStackPageFrame == NULL) {
			processManagerReleaseProcessResources(process);
			return NULL;
		}
		process->systemStack = (void*) memoryManagerGetPageFramePhysicalAddress(systemStackPageFrame);

		struct DoubleLinkedListElement* pageDirectoryPageFrame = memoryManagerAcquirePageFrame(true, -1);
		if (pageDirectoryPageFrame == NULL) {
			processManagerReleaseProcessResources(process);
			return NULL;
		}
		doubleLinkedListInsertAfterLast(&process->pagingPageFramesList, pageDirectoryPageFrame);

		uint16_t codeSegmentSelector = x86SegmentSelector(USER_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX, false, 3);
		uint16_t dataSegmentSelector = x86SegmentSelector(USER_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX, false, 3);
		uint32_t eflags = EFLAGS_RESERVED | EFLAGS_INTERRUPT_ENABLE_FLAG_MASK;

		uint32_t* systemStack = (uint32_t*) process->systemStack;
		uint32_t esp0 = PAGE_FRAME_SIZE / sizeof(uint32_t);
		systemStack[--esp0] = dataSegmentSelector; /* SS */
		systemStack[--esp0] = STACK_SEGMENT_FIRST_INVALID_VIRTUAL_ADDRESS_AFTER; /* ESP */
		systemStack[--esp0] = eflags; /* EFLAGS */
		systemStack[--esp0] = codeSegmentSelector; /* CS */
		systemStack[--esp0] = CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS; /* EIP */

		process->processExecutionState1 = (void*) (process->systemStack + esp0 * sizeof(uint32_t));
		systemStack[--esp0] = 0; /* Is there an error code? */
		systemStack[--esp0] = 0; /* Dummy error code */

		uint32_t espValue = (uint32_t) (process->systemStack + esp0 * sizeof(uint32_t));
		systemStack[--esp0] = 0; /* EAX */
		systemStack[--esp0] = 0; /* ECX */
		systemStack[--esp0] = 0; /* EDX */
		systemStack[--esp0] = 0; /* EBX */
		systemStack[--esp0] = espValue; /* ESP */
		systemStack[--esp0] = 0; /* EBP */
		systemStack[--esp0] = 0; /* ESI */
		systemStack[--esp0] = 0; /* EDI */

		systemStack[--esp0] = dataSegmentSelector; /* DS */

		process->processExecutionState2 = (void*) (process->systemStack + esp0 * sizeof(uint32_t));
		systemStack[--esp0] = dataSegmentSelector;
		systemStack[--esp0] = dataSegmentSelector;
		systemStack[--esp0] = dataSegmentSelector;

		systemStack[--esp0] = (uint32_t) argument;
		systemStack[--esp0] = (uint32_t) initializationCallback;

		process->tssSegmentDescriptor = x86SegmentDescriptor((uint32_t) &process->x86TaskState, sizeof(struct X86TaskState) - 1,
			SEGMENT_DESCRIPTOR_PRESENT | SEGMENT_DESCRIPTOR_DPL_0 | SEGMENT_DESCRIPTOR_GRANULARITY_1_BYTE | SEGMENT_DESCRIPTOR_TYPE_32_BIT_TSS_AVAILABLE | SEGMENT_DESCRIPTOR_32_BIT);

		codeSegmentSelector = x86SegmentSelector(SYSTEM_KERNEL_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX, false, 0);
		dataSegmentSelector = x86SegmentSelector(SYSTEM_KERNEL_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX, false, 0);
		eflags = EFLAGS_RESERVED;

		/*
		 * The task first instruction will be "processStart".
		 */
		esp0 = (uint32_t) (process->systemStack + esp0 * sizeof(uint32_t));
		struct X86TaskState* x86TaskState = &process->x86TaskState;
		initializeX86TaskState(x86TaskState, codeSegmentSelector, dataSegmentSelector,
			memoryManagerGetPageFramePhysicalAddress(pageDirectoryPageFrame),
			(uint32_t) (process->systemStack + PAGE_FRAME_SIZE),
			(uint32_t) &processStart, esp0, eflags);

		return process;

	} else {
		return NULL;
	}
}

static bool configureListMapping(struct DoubleLinkedList* pageFramesList, struct DoubleLinkedList* pagingPageFramesList, uint32_t* pageDirectory, uint32_t virtualAddress, bool addressGrowsUpward , uint32_t flags) {
	struct DoubleLinkedListElement* pageFrame = doubleLinkedListFirst(pageFramesList);
	while (pageFrame != NULL) {
		struct DoubleLinkedListElement* newPageFrame;
		uint32_t physicalAddress = memoryManagerGetPageFramePhysicalAddress(pageFrame);
		if (!memoryManagerConfigureMapping(&newPageFrame, pageDirectory, virtualAddress, physicalAddress, flags)) {
			return false;
		}
		if (newPageFrame != NULL) {
			doubleLinkedListInsertAfterLast(pagingPageFramesList, newPageFrame);
		}

		pageFrame = pageFrame->next;
		if (addressGrowsUpward) {
			virtualAddress += PAGE_FRAME_SIZE;
		} else {
			virtualAddress -= PAGE_FRAME_SIZE;
		}
	}

	return true;
}

static bool configureProcessMapping(struct Process* process) {
	struct X86TaskState* x86TaskState = &process->x86TaskState;

	/* It maps each virtual address to its corresponding physical address. */
	uint32_t* pageDirectory = (uint32_t*) x86TaskState->cr3;
	initializeSystemEntriesOfPageDirectory(pageDirectory, SYSTEM_PAGE_TABLES_COUNT);

	uint32_t flags = PAGE_ENTRY_PRESENT | PAGE_ENTRY_READ_WRITE | PAGE_ENTRY_USER | PAGE_ENTRY_CACHE_ENABLED |
			PAGE_ENTRY_SIZE_4_KBYTES | PAGE_ENTRY_LOCAL;

	/* Code segment: */
	if (!configureListMapping(&process->codeSegmentPageFramesList, &process->pagingPageFramesList, pageDirectory, CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS, true, flags)) {
		return false;
	}

	/* Stack segment: */
	if (!configureListMapping(&process->stackSegmentPageFramesList, &process->pagingPageFramesList, pageDirectory, STACK_SEGMENT_FIRST_INVALID_VIRTUAL_ADDRESS_AFTER - (STACK_SEGMENT_FIRST_INVALID_VIRTUAL_ADDRESS_AFTER % PAGE_FRAME_SIZE), false, flags)) {
		return false;
	}

	/* Data segment: */
	if (!configureListMapping(&process->dataSegmentPageFramesList, &process->pagingPageFramesList, pageDirectory, DATA_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS, true, flags)) {
		return false;
	}

	return true;
}

static bool copyPageFrameList(struct DoubleLinkedList* sourceDoubleLinkedList, struct DoubleLinkedList* destinationDoubleLinkedList) {
	struct DoubleLinkedListElement* pageFrame = doubleLinkedListFirst(sourceDoubleLinkedList);
	while (pageFrame != NULL) {
		struct DoubleLinkedListElement* newPageFrame = memoryManagerAcquirePageFrame(false, -1);
		if (newPageFrame == NULL) {
			return false;
		}
		doubleLinkedListInsertAfterLast(destinationDoubleLinkedList, newPageFrame);

		void* destination = (void*) memoryManagerGetPageFramePhysicalAddress(newPageFrame);
		void* source = (void*) memoryManagerGetPageFramePhysicalAddress(pageFrame);
		memcpy(destination, source, PAGE_FRAME_SIZE);

		pageFrame = pageFrame->next;
	}

	assert(doubleLinkedListSize(sourceDoubleLinkedList) == doubleLinkedListSize(destinationDoubleLinkedList));

	return true;
}

APIStatusCode processManagerForkProcess(struct Process* parentProcess, struct Process** childProcess) {
	struct ProcessExecutionState1* parentProcessExecutionState1 = parentProcess->processExecutionState1;
	struct ProcessExecutionState2* parentProcessExecutionState2 = parentProcess->processExecutionState2;

	*childProcess = NULL;
	struct Process* process = doCreateProcess(NULL, NULL);
	if (process == NULL) {
		return ENOMEM;
	}
	struct ProcessExecutionState1* processExecutionState1 = process->processExecutionState1;
	struct ProcessExecutionState2* processExecutionState2 = process->processExecutionState2;

	assert(process->parentProcess == NULL);

	{
		APIStatusCode result = SUCCESS;
		/* It copies each page frame. */
		/* Before doing it, it needs a 1 to 1 paging mapping. It is not efficient and it needs to be improved. */
		uint32_t oldCR3 = x86GetCR3();
		assert(oldCR3 != (uint32_t) systemX86TaskPageDirectory);
		x86SetCR3((uint32_t) systemX86TaskPageDirectory);
		if (result == SUCCESS && !copyPageFrameList(&parentProcess->codeSegmentPageFramesList, &process->codeSegmentPageFramesList)) {
			result = ENOMEM;
		}
		if (result == SUCCESS && !copyPageFrameList(&parentProcess->dataSegmentPageFramesList, &process->dataSegmentPageFramesList)) {
			result = ENOMEM;
		}
		if (result == SUCCESS && !copyPageFrameList(&parentProcess->stackSegmentPageFramesList, &process->stackSegmentPageFramesList)) {
			result = ENOMEM;
		}
		x86SetCR3(oldCR3);

		if (result != SUCCESS) {
			processManagerReleaseProcessResources(process);
			return result;
		}
	}

	if (!configureProcessMapping(process)) {
		processManagerReleaseProcessResources(process);
		return ENOMEM;
	}

	process->parentProcess = parentProcess;

	processExecutionState2->edi = parentProcessExecutionState2->edi;
	processExecutionState2->esi = parentProcessExecutionState2->esi;
	processExecutionState2->ebp = parentProcessExecutionState2->ebp;
	processExecutionState1->esp3 = parentProcessExecutionState1->esp3;
	processExecutionState2->ebx = 0;
	processExecutionState2->edx = parentProcessExecutionState2->edx;
	processExecutionState2->ecx = parentProcessExecutionState2->ecx;
	processExecutionState2->eax = SUCCESS;
	processExecutionState1->eip = parentProcessExecutionState1->eip;
	processExecutionState1->eflags = parentProcessExecutionState1->eflags;

	strcpy(process->currentWorkingDirectory, parentProcess->currentWorkingDirectory);
	process->currentWorkingDirectoryLength = parentProcess->currentWorkingDirectoryLength;

	process->fileModeCreationMask = parentProcess->fileModeCreationMask;

	for (int i = 0; i < MAX_FILE_DESCRIPTORS_PER_PROCESS; i++) {
		struct FileDescriptor* fileDescriptor = &parentProcess->fileDescriptors[i];
		struct OpenFileDescription* openFileDescription = fileDescriptor->openFileDescription;

		if (openFileDescription != NULL) {
			process->fileDescriptors[i].openFileDescription = openFileDescription;
			process->fileDescriptors[i].flags = fileDescriptor->flags;
			openFileDescription->usageCount++;
		}
	}

	doubleLinkedListInsertAfterLast(&runnableProcessesList, &process->runnableProcessListElement);
	bool result = fixedCapacitySortedArrayInsert(&allProcessesArray, &process);
	assert(result == true);
	doubleLinkedListInsertAfterLast(&parentProcess->childrenProcessList, &process->childrenProcessListElement);

	memcpy(&process->blockedSignalsSet, &parentProcess->blockedSignalsSet, sizeof(sigset_t));
	memcpy(process->signalInformation, parentProcess->signalInformation, sizeof(struct SignalInformation) * NUMBER_OF_SIGNALS);
	for (int signalId = 1; signalId <= NUMBER_OF_SIGNALS; signalId++) {
		struct SignalInformation* signalInformation = &process->signalInformation[signalId - 1];
		assert(!signalInformation->pending || !signalInformation->signalCreationInformation.inResponseToUnrecoverableFault);
		signalInformation->pending = false;
	}

	process->fpuInitialized = parentProcess->fpuInitialized;
	if (parentProcess->fpuInitialized) {
		memcpy(process->fpuState, parentProcess->fpuState, X86_FPU_STATE_LENGTH);
	}

	if (parentProcess->processGroup != NULL) {
		assert(process->processGroup == NULL);
		processGroupInsertProcess(parentProcess->processGroup, process);
	}

	*childProcess = process;
	return SUCCESS;
}

static bool changeSegmentSize(struct DoubleLinkedList* segmentPageFramesList, struct DoubleLinkedList* pagingPageFramesList, size_t size,
		uint32_t* pageDirectory, uint32_t firstVirtualAddress) {
	struct DoubleLinkedList newSegmentPageFramesList;
	doubleLinkedListInitialize(&newSegmentPageFramesList);
	struct DoubleLinkedList newPagingPageFramesList;
	doubleLinkedListInitialize(&newPagingPageFramesList);

	bool result = true;
	uint32_t pageFramesCount = mathUtilsCeilOfUint32Division(size, PAGE_FRAME_SIZE);
	if (pageFramesCount > doubleLinkedListSize(segmentPageFramesList)) {
		/* First, it allocates the necessary page frames. */
		for (int i = doubleLinkedListSize(segmentPageFramesList); i < pageFramesCount; i++) {
			struct DoubleLinkedListElement* pageFrame = memoryManagerAcquirePageFrame(false, -1);
			if (pageFrame == NULL) {
				result = false;
				break;
			}
			doubleLinkedListInsertAfterLast(&newSegmentPageFramesList, pageFrame);
		}

		if (result) {
			/* There was enough memory.  */

			uint32_t flags = PAGE_ENTRY_PRESENT | PAGE_ENTRY_READ_WRITE | PAGE_ENTRY_USER | PAGE_ENTRY_CACHE_ENABLED |
							PAGE_ENTRY_SIZE_4_KBYTES | PAGE_ENTRY_LOCAL;

			uint32_t virtualAddress = firstVirtualAddress + doubleLinkedListSize(segmentPageFramesList) * PAGE_FRAME_SIZE;
			struct DoubleLinkedListElement* pageFrame = doubleLinkedListFirst(&newSegmentPageFramesList);
			while (pageFrame != NULL) {
				struct DoubleLinkedListElement* newPageFrame;
				uint32_t physicalAddress = memoryManagerGetPageFramePhysicalAddress(pageFrame);
				if (memoryManagerConfigureMapping(&newPageFrame, pageDirectory, virtualAddress, physicalAddress, flags)) {
					if (newPageFrame != NULL) {
						doubleLinkedListInsertAfterLast(&newPagingPageFramesList, newPageFrame);
					}

				} else {
					result = false;
					break;
				}

				virtualAddress += PAGE_FRAME_SIZE;
				pageFrame = pageFrame->next;
			}

			if (result) {
				/* Invalidate the necessary TLB entries. */
				virtualAddress = firstVirtualAddress + doubleLinkedListSize(segmentPageFramesList) * PAGE_FRAME_SIZE;
				pageFrame = doubleLinkedListFirst(&newSegmentPageFramesList);
				while (pageFrame != NULL) {
					x86InvalidateTLBEntry(virtualAddress);
					virtualAddress += PAGE_FRAME_SIZE;
					pageFrame = pageFrame->next;
				}

			} else {
				/* There was not enough memory.  */

				/* Undo all memory mappings. */
				virtualAddress = firstVirtualAddress + doubleLinkedListSize(segmentPageFramesList) * PAGE_FRAME_SIZE;
				pageFrame = doubleLinkedListFirst(&newSegmentPageFramesList);
				while (pageFrame != NULL) {
					memoryManagerRemovePageMapping(pageDirectory, virtualAddress, memoryManagerGetPageFramePhysicalAddress(pageFrame));
					virtualAddress += PAGE_FRAME_SIZE;
					pageFrame = pageFrame->next;
				}

				pageFrame = doubleLinkedListFirst(&newPagingPageFramesList);
				while (pageFrame != NULL) {
					uint32_t physicalAddress = memoryManagerGetPageFramePhysicalAddress(pageFrame);
					memoryManagerRemovePageTableMapping(pageDirectory, physicalAddress);
					pageFrame = pageFrame->next;
				}
			}
		}

		if (result) {
			doubleLinkedListInsertListAfterLast(segmentPageFramesList, &newSegmentPageFramesList);
			doubleLinkedListInsertListAfterLast(pagingPageFramesList, &newPagingPageFramesList);

		} else {
			releasePageFrameList(&newSegmentPageFramesList);
			releasePageFrameList(&newPagingPageFramesList);
		}

	} else {
		/* It should release memory. */

		uint32_t virtualAddress = firstVirtualAddress + doubleLinkedListSize(segmentPageFramesList) * PAGE_FRAME_SIZE;
		while (pageFramesCount < doubleLinkedListSize(segmentPageFramesList)) {
			virtualAddress -= PAGE_FRAME_SIZE;
			assert(firstVirtualAddress <= virtualAddress);

			struct DoubleLinkedListElement* pageFrame = doubleLinkedListRemoveLast(segmentPageFramesList);
			memoryManagerRemovePageMapping(pageDirectory, virtualAddress, memoryManagerGetPageFramePhysicalAddress(pageFrame));
			memoryManagerReleasePageFrame(pageFrame, -1);
			x86InvalidateTLBEntry(virtualAddress);
		}

		// TODO: We are not releasing the page frames used to store page tables.
	}

	return result;
}

APIStatusCode processManagerCreateInitProcess( __attribute__ ((cdecl)) void (*initializationCallback)(void*), void* argument) {
	APIStatusCode result = SUCCESS;
	struct Process* process = doCreateProcess(initializationCallback, argument);
	if (process == NULL) {
		result = ENOMEM;
	}

	/* We do create a session and a process group to make sure all process will inherit a existent session and process group. */
	if (result == SUCCESS) {
		result = processServicesCreateSessionAndProcessGroup(process);
	}

	if (result == SUCCESS) {
		strcpy(process->currentWorkingDirectory, "/");
		process->currentWorkingDirectoryLength = 1;

		for (int i = 0; i < STACK_PAGE_FRAME_COUNT; i++) {
			struct DoubleLinkedListElement* stackPageFrame = memoryManagerAcquirePageFrame(false, -1);
			if (stackPageFrame == NULL) {
				result = ENOMEM;
				break;
			}
			doubleLinkedListInsertAfterLast(&process->stackSegmentPageFramesList, stackPageFrame);
		}

		if (result == SUCCESS) {
			if (!configureProcessMapping(process)) {
				result = ENOMEM;
			}

			doubleLinkedListInsertAfterLast(&runnableProcessesList, &process->runnableProcessListElement);
			bool insertionResult = fixedCapacitySortedArrayInsert(&allProcessesArray, &process);
			assert(insertionResult);

			initProcess = process;
		}
	}

	if (result != SUCCESS) {
		processManagerReleaseProcessResources(process);
	}

	return result;
}

void processManagerStartScheduling(void) {
	pitRegisterCommandToRunOnTick(&scheduleProcessExecutionAfterInterruptionHandler);

	currentProcess = NULL;

	/* It enables the "Page Global" feature and "Time-Stamp" feature. */
	x86SetCR4((x86GetCR4() | (1 << 7)) & ~0x4);

	__asm__ __volatile__(
				"pushl %%eax;" /* EFLAGS. */
				"pushl %%ebx;" /* CS. */
				"pushl $userMode;" /* EIP. */
				"iret;"
				"userMode: nop"
			:
			: "a"(systemX86TaskState.eflags), "b"(systemX86TaskState.cs)
			: "cc", "memory");

	logDebug("The system idle task has started its execution");

	/* It now enables the interruptions. */
	x86Sti();

	while (true) {
		x86Hlt();
	}
}

static const pid_t* processIdExtractor(const struct Process** process) {
	return &(*process)->id;
}

static int processIdComparator(const pid_t* processId1, const pid_t* processId2) {
	return *processId1 - *processId2;
}

static const pid_t* processGroupIdExtractor(const struct ProcessGroup** processGroup) {
	return &(*processGroup)->id;
}

static pid_t lastProcessIdThatUsedFPU = 0;
static void handleFirstFPUInstructionAfterSchedule(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	struct Process* process = currentProcess;
	assert(process != NULL);

	x86ClearTaskSwitchedFlag();

	if (lastProcessIdThatUsedFPU != 0) {
		struct Process* lastProcess = processManagerGetProcessById(lastProcessIdThatUsedFPU);
		if (lastProcess != NULL && lastProcess->fpuInitialized) {
			x86Fxsave(lastProcess->fpuState);
		}
	}

	lastProcessIdThatUsedFPU = process->id;

	if (process->fpuInitialized) {
		x86Fxrstor(process->fpuState);
	} else {
		process->fpuInitialized = true;
		x86Fninit();
	}
}

APIStatusCode processManagerInitialize(void) {
	APIStatusCode result = SUCCESS;

	struct DoubleLinkedListElement* possibleOrphanedProcessGroupsArrayPageFrame = NULL;
	if (result == SUCCESS) {
		possibleOrphanedProcessGroupsArrayPageFrame = memoryManagerAcquirePageFrame(true, -1);
		if (possibleOrphanedProcessGroupsArrayPageFrame != NULL) {
			fixedCapacitySortedArrayInitialize(&possibleOrphanedProcessGroupsArray, sizeof(struct Process*),
					(void*) memoryManagerGetPageFramePhysicalAddress(possibleOrphanedProcessGroupsArrayPageFrame),
					PAGE_FRAME_SIZE, 	(int (*)(const void*, const void*)) &processIdComparator,
					(const void* (*)(const void*)) &processGroupIdExtractor);
		} else {
			result = ENOMEM;
		}
	}

	struct DoubleLinkedListElement* allProcessesArrayPageFrame = NULL;
	if (result == SUCCESS) {
		allProcessesArrayPageFrame = memoryManagerAcquirePageFrame(true, -1);
		if (allProcessesArrayPageFrame != NULL) {
			fixedCapacitySortedArrayInitialize(&allProcessesArray, sizeof(struct Process*),
					(void*) memoryManagerGetPageFramePhysicalAddress(allProcessesArrayPageFrame),
					PAGE_FRAME_SIZE, 	(int (*)(const void*, const void*)) &processIdComparator,
					(const void* (*)(const void*)) &processIdExtractor);
			doubleLinkedListInitialize(&runnableProcessesList);

			uint16_t codeSegmentSelector = x86SegmentSelector(SYSTEM_KERNEL_LINEAR_CODE_SEGMENT_DESCRIPTOR_INDEX, false, 0);
			uint16_t dataSegmentSelector = x86SegmentSelector(SYSTEM_KERNEL_LINEAR_DATA_SEGMENT_DESCRIPTOR_INDEX, false, 0);
			uint32_t eflags = EFLAGS_RESERVED;
			initializeSystemEntriesOfPageDirectory(systemX86TaskPageDirectory, memoryManagerGetSystemPageTablesCount());
			initializeX86TaskState(&systemX86TaskState, codeSegmentSelector, dataSegmentSelector,
				(uint32_t) &systemX86TaskPageDirectory, (uint32_t) INITIALIZATION_STACK_TOP, 0, 0, eflags);

			systemX86TaskTSSSegmentDescriptor = x86SegmentDescriptor((uint32_t) &systemX86TaskState, sizeof(struct X86TaskState) - 1,
				SEGMENT_DESCRIPTOR_PRESENT | SEGMENT_DESCRIPTOR_DPL_0 | SEGMENT_DESCRIPTOR_GRANULARITY_1_BYTE | SEGMENT_DESCRIPTOR_TYPE_32_BIT_TSS_AVAILABLE | SEGMENT_DESCRIPTOR_32_BIT);
			x86WriteOnGDT(TASK_STATE_SEGMENT_DESCRIPTOR_INDEX, systemX86TaskTSSSegmentDescriptor);

			uint16_t segmentSelector = x86SegmentSelector(TASK_STATE_SEGMENT_DESCRIPTOR_INDEX, false, 0);
			__asm__ __volatile__(
				"ltr %%ax;"
				:
				: "a"(segmentSelector)
				: "memory");

			/* It now enables the paging and the cache. */
			__asm__ __volatile__(
				"mov %%eax, %%cr3;"
				"mov %%cr0, %%eax;"
				"or $0x80000000, %%eax;" /* Paging (bit 31 of CR0) <- true */
				"and $0x9FFFFFFF, %%eax;" /*
													* Cache Disable (bit 30 of CR0) <- false
													* Not Write-through (bit 29 of CR0) <- false
													*/
				"mov %%eax, %%cr0;"
				 :
				 : "a"(systemX86TaskState.cr3)
				 : "cc", "memory");

			interruptionManagerRegisterInterruptionHandler(X86_DEVICE_NOT_AVAILABLE_NO_MATH_COPROCESSOR, &handleFirstFPUInstructionAfterSchedule);

		} else {
			result = ENOMEM;
		}
	}

	if (result != SUCCESS) {
		if (possibleOrphanedProcessGroupsArrayPageFrame != NULL) {
			memoryManagerReleasePageFrame(possibleOrphanedProcessGroupsArrayPageFrame, -1);
		}
		if (allProcessesArrayPageFrame != NULL) {
			memoryManagerReleasePageFrame(allProcessesArrayPageFrame, -1);
		}
	}

	return result;
}

__attribute__ ((cdecl)) struct Process* processManagerGetCurrentProcess(void) {
	return currentProcess;
}

APIStatusCode processManagerChangeDataSegmentSize(struct Process* process, int increment) {
	APIStatusCode result = SUCCESS;

	uint32_t* pageDirectory = (uint32_t*) process->x86TaskState.cr3;
	size_t currentSize = doubleLinkedListSize(&process->dataSegmentPageFramesList) * PAGE_FRAME_SIZE;
	if (increment > 0) {
		if (!changeSegmentSize(&process->dataSegmentPageFramesList, &process->pagingPageFramesList, currentSize + increment,
				pageDirectory, DATA_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS)) {
			result = ENOMEM;
		}

	} else if (increment < 0) {
		size_t size = currentSize;
		if (currentSize > -increment) {
			size += increment;
		} else {
			size = 0;
		}
		changeSegmentSize(&process->dataSegmentPageFramesList, &process->pagingPageFramesList, size, pageDirectory, DATA_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS);
		assert(mathUtilsCeilOfUint32Division(size, PAGE_FRAME_SIZE) == doubleLinkedListSize(&process->dataSegmentPageFramesList));
	}

	return result;
}

APIStatusCode processManagerChangeCodeSegmentSize(struct Process* process, size_t executableSize, bool allowShrinkingIfNecessary) {
	APIStatusCode result = SUCCESS;

	if (allowShrinkingIfNecessary || executableSize >= doubleLinkedListSize(&process->codeSegmentPageFramesList) * PAGE_FRAME_SIZE) {
		uint32_t* pageDirectory = (uint32_t*) process->x86TaskState.cr3;
		if (!changeSegmentSize(&process->codeSegmentPageFramesList, &process->pagingPageFramesList, executableSize, pageDirectory, CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS)) {
			result = ENOMEM;
		}
	}

	return result;
}

struct Process* processManagerGetProcessById(pid_t id) {
	struct Process** result = fixedCapacitySortedArraySearch(&allProcessesArray, &id);
	if (result == NULL) {
		return NULL;
	} else {
		return *result;
	}
}

static void appendSegmentInformation(struct StringStreamWriter* stringStreamWriter, const char* segmentName, struct ProcessMemorySegmentLimits* processMemorySegmentLimits,
		struct DoubleLinkedList* segmentPageFramesList) {
	streamWriterFormat(&stringStreamWriter->streamWriter, "    %s:\n", segmentName);
	streamWriterFormat(&stringStreamWriter->streamWriter, "      segmentPageFramesList: %d\n", doubleLinkedListSize(segmentPageFramesList));
	if (!processMemorySegmentLimits->isEmpty) {
		streamWriterFormat(&stringStreamWriter->streamWriter, "      limits: from %X to %X (exclusive)\n", processMemorySegmentLimits->firstAddress, processMemorySegmentLimits->lastAddress);
		streamWriterFormat(&stringStreamWriter->streamWriter, "      size (KB): %u\n", doubleLinkedListSize(segmentPageFramesList) * PAGE_FRAME_SIZE / 1024);
	}
}

APIStatusCode processManagerPrintDebugReport(void) {
	const int bufferSize = 1024;
	char buffer[bufferSize];
	struct StringStreamWriter stringStreamWriter;

	logDebug("Process manager report:\n");

	for (int i = 0; i < fixedCapacitySortedArraySize(&allProcessesArray); i++) {
		struct Process* process = *((struct Process**) fixedCapacitySortedArrayGet(&allProcessesArray, i));
		struct ProcessMemorySegmentsLimits processMemorySegmentsLimits;
		processGetProcessMemorySegmentsLimits(process, &processMemorySegmentsLimits, false);

		struct Session* session = processGetSession(process);

		stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);
		streamWriterFormat(&stringStreamWriter.streamWriter, "Process %d - %s\n", process->id, PROCESS_STATE_NAMES[process->state]);

		streamWriterFormat(&stringStreamWriter.streamWriter, "  parent: %d", (process->parentProcess != NULL ? process->parentProcess->id : 0));
		streamWriterFormat(&stringStreamWriter.streamWriter, " session: %d", session->id);
		streamWriterFormat(&stringStreamWriter.streamWriter, " processGroup: %d", process->processGroup->id);
		streamWriterFormat(&stringStreamWriter.streamWriter, " controllingTTYId: %d\n", session->controllingTTYId);

		streamWriterFormat(&stringStreamWriter.streamWriter, "  blocked signal set: %llX", process->blockedSignalsSet);

		uint64_t pendingSignals = 0;
		for (int signalId = 1; signalId <= NUMBER_OF_SIGNALS; signalId++) {
			if (process->signalInformation[signalId - 1].pending) {
				pendingSignals |= ((uint64_t) 1) << signalId;
			}
		}
		streamWriterFormat(&stringStreamWriter.streamWriter, " pending signals: %llX\n", pendingSignals);

		streamWriterFormat(&stringStreamWriter.streamWriter, "  file descriptors:");
		for (int i = 0; i < MAX_FILE_DESCRIPTORS_PER_PROCESS; i++) {
			struct FileDescriptor* fileDescriptor = &process->fileDescriptors[i];
			struct OpenFileDescription* openFileDescription = fileDescriptor->openFileDescription;
			if (openFileDescription != NULL) {
				streamWriterFormat(&stringStreamWriter.streamWriter, " %d (%X)", i, openFileDescription);
			}
		}
		streamWriterFormat(&stringStreamWriter.streamWriter, "\n");
		streamWriterFormat(&stringStreamWriter.streamWriter, "  segments:\n");
		appendSegmentInformation(&stringStreamWriter, "code", &processMemorySegmentsLimits.code, &process->codeSegmentPageFramesList);
		appendSegmentInformation(&stringStreamWriter, "data", &processMemorySegmentsLimits.data, &process->dataSegmentPageFramesList);
		appendSegmentInformation(&stringStreamWriter, "stack", &processMemorySegmentsLimits.stack, &process->stackSegmentPageFramesList);
		stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

		logDebug("%s", buffer);
	}

	return SUCCESS;
}

static void* transformValueBeforeNextReturn(struct Iterator* iterator, void* value) {
	return *((void **) value);
}

void processManagerInitializeAllProcessesIterator(struct FixedCapacitySortedArrayIterator* fixedCapacitySortedArrayIterator) {
	fixedCapacitySortedArrayInitializeIterator(&allProcessesArray, fixedCapacitySortedArrayIterator);
	fixedCapacitySortedArrayIterator->iterator.transformValueBeforeNextReturn = &transformValueBeforeNextReturn;
}
