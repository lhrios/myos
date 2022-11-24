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

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kernel/interruption_manager.h"
#include "kernel/log.h"
#include "kernel/x86.h"

#include "kernel/process/process_manager.h"

#include "kernel/services/signal_services.h"

#include "util/priority_queue.h"
#include "util/string_stream_writer.h"

struct InterruptionHandler {
	void (*handler)(uint32_t, struct ProcessExecutionState1*, struct ProcessExecutionState2*);
};
uint32_t interruptionManagerInterruptionVectorToHandleSystemCall;
static void (*systemCallHandler)(struct ProcessExecutionState1*, struct ProcessExecutionState2*);
static struct InterruptionHandler registeredInterruptionHandlers[X86_INTERRUPT_VECTOR_COUNT];

struct CommandToRunAfterInterruptionHandler {
	int priority;
	void (*command)();
	void* argument;
};

#define REGISTERED_COMMANDS_TO_RUN_AFTER_INTERRUPTION_HANDLER_ARRAY_LENGTH 8
struct CommandToRunAfterInterruptionHandler registeredCommandsToRunAfterInterruption[REGISTERED_COMMANDS_TO_RUN_AFTER_INTERRUPTION_HANDLER_ARRAY_LENGTH];
static struct CommandToRunAfterInterruptionHandler* queue[REGISTERED_COMMANDS_TO_RUN_AFTER_INTERRUPTION_HANDLER_ARRAY_LENGTH];
static struct PriorityQueue priorityQueue;

static bool scheduledCommandComparator(const void* command1, const void* command2, void* argument1, void* argument2, void* argument3) {
	return (*((struct CommandToRunAfterInterruptionHandler**) command1))->priority > (*((struct CommandToRunAfterInterruptionHandler**) command2))->priority;
}

static void appendErrorCodeInformation(struct StringStreamWriter* stringStreamWriter, uint32_t errorCode) {
	if (errorCode & 0x1) {
		streamWriterFormat(&stringStreamWriter->streamWriter, "  It was caused by an event external to the program.\n");
	} else {
		streamWriterFormat(&stringStreamWriter->streamWriter, "  It was caused by the program.\n");
	}

	uint32_t index = (0xFFF8 & errorCode) >> 3;
	if (errorCode & 0x2) {
		streamWriterFormat(&stringStreamWriter->streamWriter, "  The index (%X) refers to a gate descriptor in the IDT.\n", index);
	}else{
		if (errorCode & 0x4){
			streamWriterFormat(&stringStreamWriter->streamWriter, "  The index (%X) refers to a descriptor in the LDT.\n", index);
		}else{
			streamWriterFormat(&stringStreamWriter->streamWriter, "  The index (%X) refers to a descriptor in the GDT.\n", index);
		}
	}
}

static void defaultInterruptionHandler(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	const int BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	streamWriterFormat(&stringStreamWriter.streamWriter, "An unexpected interruption has been triggered: ");
	if (processExecutionState2->interruptionVector >= X86_FIRST_NON_RESERVED_INTERRUPT_VECTOR) {
		streamWriterFormat(&stringStreamWriter.streamWriter, "non-reserved interruption vector (%d).\n", processExecutionState2->interruptionVector);
	} else {
		streamWriterFormat(&stringStreamWriter.streamWriter, "%s.\n", X86_INTERRUPTIONS_NAMES[processExecutionState2->interruptionVector]);
	}

	if (processExecutionState2->interruptionVector == X86_PAGE_FAULT_INTERRUPT_VECTOR) {
		streamWriterFormat(&stringStreamWriter.streamWriter, "  CR2=%.8X\n", x86GetCR2());

		if (errorCode & PAGE_FAULT_EXCEPTION_PAGE_LEVEL_PROTECTION_VIOLATION) {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The fault was caused by a page-level protection violation.\n");
		} else {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The fault was caused by a non-present page.\n");
		}

		if (errorCode & PAGE_FAULT_EXCEPTION_WRITE) {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The access causing the fault was a write.\n");
		} else {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The access causing the fault was a read.\n");
		}

		if (errorCode & PAGE_FAULT_EXCEPTION_USER_MODE) {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The access causing the fault originated when the processor was executing in user mode.\n");
		} else {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The access causing the fault originated when the processor was executing in supervisor mode.\n");
		}

		if (errorCode & PAGE_FAULT_EXCEPTION_WAS_RESERVED_BIT_VIOLATION) {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The fault was caused by reserved bits set to 1 in a page directory.\n");
		} else {
			streamWriterFormat(&stringStreamWriter.streamWriter, "  The fault was not caused by reserved bit violation.\n");
		}

	} else if (x86InterruptionHasErrorCode(processExecutionState2->interruptionVector)) {
		appendErrorCodeInformation(&stringStreamWriter, errorCode);
	}

	uint8_t clpWasExecuting = x86GetSegmentSelectorRPL(processExecutionState1->cs);
	struct Process* currentProcess = processManagerGetCurrentProcess();
	assert(clpWasExecuting == 0 || currentProcess != NULL);

	if (currentProcess != NULL) {
		streamWriterFormat(&stringStreamWriter.streamWriter, "  While executing the process whose id is %u.\n", currentProcess->id);
	}

	streamWriterFormat(&stringStreamWriter.streamWriter, "  eax=%.8X ebx=%.8X ecx=%.8X edx=%.8X\n", processExecutionState2->eax, processExecutionState2->ebx,
			processExecutionState2->ecx, processExecutionState2->edx);
	uint32_t esp;
	if (clpWasExecuting == 3) {
		esp = processExecutionState1->esp3;
	} else {
		esp = processExecutionState2->esp0;
	}
	streamWriterFormat(&stringStreamWriter.streamWriter, "  esi=%.8X edi=%.8X eip=%.8X esp=%.8X ebp=%.8X\n", processExecutionState2->esi, processExecutionState2->edi,
			processExecutionState1->eip, esp, processExecutionState2->ebp);
	if (clpWasExecuting == 3) {
		streamWriterFormat(&stringStreamWriter.streamWriter, "  cs=%.4X ss=%.4X eflags=%.8X\n", processExecutionState1->cs, processExecutionState1->ss, processExecutionState1->eflags);
	} else {
		streamWriterFormat(&stringStreamWriter.streamWriter, "  cs=%.4X ds=%.4X ss=%.4X eflags=%.8X\n", x86GetCS(), processExecutionState2->ds, x86GetSS(), processExecutionState1->eflags);
	}
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);

	logDebug("%s", buffer);

	assert(currentProcess != NULL);

	currentProcess->processExecutionState1 = processExecutionState1;
	currentProcess->processExecutionState2 = processExecutionState2;

	int signalId;
	switch(processExecutionState2->interruptionVector) {
		case X86_DIVIDE_ERROR_INTERRUPT_VECTOR:
			signalId = SIGFPE;
			break;

		case X86_INVALID_OPCODE_INTERRUPT_VECTOR:
			signalId = SIGILL;
			break;

		default:
			signalId = SIGSEGV;
			break;
	}

	assert(clpWasExecuting != 3 || currentProcess->state == RUNNABLE);
	signalServicesGenerateSignal(currentProcess, currentProcess->id, signalId, clpWasExecuting != 3, NULL);
}

bool interruptionManagerRegisterCommandToRunAfterInterruptionHandler(int priority, void (*command)(void*), void* argument) {
	int size = priorityQueueSize(&priorityQueue);
	if (size + 1 < REGISTERED_COMMANDS_TO_RUN_AFTER_INTERRUPTION_HANDLER_ARRAY_LENGTH) {
		struct CommandToRunAfterInterruptionHandler* commandToRunAfterInterruptionHandler = queue[size];
		commandToRunAfterInterruptionHandler->priority = priority;
		commandToRunAfterInterruptionHandler->command = command;
		commandToRunAfterInterruptionHandler->argument = argument;

		bool result = priorityQueueInsert(&priorityQueue, &commandToRunAfterInterruptionHandler);
		assert(result);
		return true;

	} else {
		return false;
	}
}

void __attribute__ ((cdecl)) interruptionManagerRunCommandsAfterInterruptionHandler(void) {
	while (priorityQueueSize(&priorityQueue) > 0) {
		struct CommandToRunAfterInterruptionHandler* commandToRunAfterInterruptionHandler;
		priorityQueueRemove(&priorityQueue, &commandToRunAfterInterruptionHandler);
		commandToRunAfterInterruptionHandler->command(commandToRunAfterInterruptionHandler->argument);
	}
}

void interruptionManagerInitialize(uint32_t interruptionVectorToHandleSystemCall) {
	x86InitializeIDT(interruptionVectorToHandleSystemCall);

	for (int interruptionVector = 0; interruptionVector < X86_INTERRUPT_VECTOR_COUNT; interruptionVector++) {
		interruptionManagerRegisterInterruptionHandler(interruptionVector, defaultInterruptionHandler);
	}

	interruptionManagerInterruptionVectorToHandleSystemCall = interruptionVectorToHandleSystemCall;

	for (int i = 0; i < REGISTERED_COMMANDS_TO_RUN_AFTER_INTERRUPTION_HANDLER_ARRAY_LENGTH; i++) {
		queue[i] = &registeredCommandsToRunAfterInterruption[i];
	}
	priorityQueueInitialize(&priorityQueue, queue, REGISTERED_COMMANDS_TO_RUN_AFTER_INTERRUPTION_HANDLER_ARRAY_LENGTH, 0,
			sizeof(struct CommandToRunAfterInterruptionHandler*), &scheduledCommandComparator, NULL, NULL, NULL);
}

void interruptionManagerRegisterInterruptionHandler(uint8_t interruptionVector,
		void (*handler)(uint32_t, struct ProcessExecutionState1*, struct ProcessExecutionState2*)) {
	struct InterruptionHandler* interruptionHandler = &registeredInterruptionHandlers[interruptionVector];
	interruptionHandler->handler = handler;
}

void interruptionManagerRegisterSystemCallHandler(void (*newSystemCallHandler)(struct ProcessExecutionState1*, struct ProcessExecutionState2*)) {
	systemCallHandler = newSystemCallHandler;
}

void __attribute__ ((cdecl)) interruptionManagerHandler(uint32_t errorCode, struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	/* As just after a signal deliver or just before process normal execution resume there is a call to handle all pending signals. */
	struct Process* process = processManagerGetCurrentProcess();
	assert(process == NULL || !process->mightHaveAnySignalToHandle);

	struct InterruptionHandler* interruptionHandler = &registeredInterruptionHandlers[processExecutionState2->interruptionVector];
	interruptionHandler->handler(errorCode, processExecutionState1, processExecutionState2);
}

void __attribute__ ((cdecl)) interruptionManagerSystemCallHandler(struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	assert(priorityQueueSize(&priorityQueue) == 0);
	assert(systemCallHandler != NULL);

	/* As just after a signal deliver or just before process normal execution resume there is a call to all handle pending signals. */
	struct Process* process = processManagerGetCurrentProcess();
	assert(process == NULL || !process->mightHaveAnySignalToHandle);

	systemCallHandler(processExecutionState1, processExecutionState2);
}
