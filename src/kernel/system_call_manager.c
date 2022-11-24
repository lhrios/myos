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
#include <string.h>
#include <stdio.h>

#include <sys/wait.h>

#include "standard_library_implementation/file_descriptor_offset_reposition_constants.h"

#include "kernel/busy_waiting_manager.h"
#include "kernel/cmos.h"
#include "kernel/command_scheduler.h"
#include "kernel/kernel_life_cycle.h"
#include "kernel/interruption_manager.h"
#include "kernel/log.h"
#include "kernel/process/process_manager.h"
#include "kernel/process/process_group_manager.h"
#include "kernel/session_manager.h"
#include "kernel/system_calls.h"
#include "kernel/system_call_manager.h"

#include "kernel/io/block_cache_manager.h"
#include "kernel/io/open_file_description.h"
#include "kernel/io/pipe_manager.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "kernel/services/debug_system_call_services.h"
#include "kernel/services/io_services.h"
#include "kernel/services/process_services.h"
#include "kernel/services/signal_services.h"

#include "util/debug_utils.h"
#include "util/math_utils.h"
#include "util/path_utils.h"

static void doExecuteExecutable(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;

	const char* executablePath = (void*) processExecutionState2->ebx;
	const char** argv = (void*) processExecutionState2->ecx;
	const char** envp = (void*) processExecutionState2->edx;

	APIStatusCode result = processServicesExecuteExecutable(currentProcess, true, executablePath, argv, envp);
	if (result != SUCCESS) {
		processExecutionState2->eax = result;
	}
}

static void doChangeSignalAction(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;

	void (*callback)(int, struct sigaction*) = (void(*)(int, struct sigaction*)) processExecutionState2->ebx;
	struct sigaction* newSignalHandlingConfiguration = (void*) processExecutionState2->edx;
	struct sigaction* oldSignalHandlingConfiguration = (void*) processExecutionState2->edi;

	if (!processIsValidSegmentAccess(currentProcess, (uint32_t) callback, sizeof(void*))
			|| (newSignalHandlingConfiguration != NULL && !processIsValidSegmentAccess(currentProcess, (uint32_t) newSignalHandlingConfiguration, sizeof(struct sigaction)))
			|| (oldSignalHandlingConfiguration != NULL && !processIsValidSegmentAccess(currentProcess, (uint32_t) oldSignalHandlingConfiguration, sizeof(struct sigaction)))) {
		processExecutionState2->eax = EFAULT;
	} else {
		processExecutionState2->eax = signalServicesChangeSignalAction(currentProcess, callback, (int)processExecutionState2->ecx, newSignalHandlingConfiguration, oldSignalHandlingConfiguration);
	}
}

static void doCreateName(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesCreateName(currentProcess, (char*) processExecutionState2->ebx, (char*) processExecutionState2->ecx, true);
}

static void doGenerateSignal(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;

	pid_t receiverProcessId = (pid_t) processExecutionState2->ebx;
	int signalId = (int) processExecutionState2->ecx;

	processExecutionState2->eax = signalServicesGenerateSignal(currentProcess, receiverProcessId, signalId, false, NULL);
}

static void doReboot(struct Process* currentProcess) {
	kernelLifeCycleReboot();
}

static void doChangeSignalBlockage(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;

	int how = (int) processExecutionState2->ebx;
	sigset_t* newSignalHandlingConfiguration = (void*) processExecutionState2->ecx;
	sigset_t* oldSignalHandlingConfiguration = (void*) processExecutionState2->edx;

	if ((newSignalHandlingConfiguration != NULL && !processIsValidSegmentAccess(currentProcess, (uint32_t) newSignalHandlingConfiguration, sizeof(sigset_t)))
			|| (oldSignalHandlingConfiguration != NULL && !processIsValidSegmentAccess(currentProcess, (uint32_t) oldSignalHandlingConfiguration, sizeof(sigset_t)))) {
		processExecutionState2->eax = EFAULT;

	} else {
		APIStatusCode result = signalServicesChangeSignalsBlockage(currentProcess, how, newSignalHandlingConfiguration, oldSignalHandlingConfiguration);
		processExecutionState2->eax = result;
		if (result == SUCCESS) {
			currentProcess->mightHaveAnySignalToHandle = true;
			signalServicesHandlePendingSignals(currentProcess);
		}
	}
}

static void doWait(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processServicesWait(currentProcess, processExecutionState2->ebx, processExecutionState2->ecx,
			(int*) &processExecutionState2->ebx, (pid_t*) &processExecutionState2->ecx);
}

static void doOpen(struct Process* currentProcess) {
	const char* path = (char*) currentProcess->processExecutionState2->ebx;
	int flags = currentProcess->processExecutionState2->ecx;
	mode_t mode = currentProcess->processExecutionState2->edx;
	int fileDescriptorIndex;

	currentProcess->processExecutionState2->eax = ioServicesOpen(currentProcess, true, path, false, flags, mode, &fileDescriptorIndex);
	currentProcess->processExecutionState2->ebx = fileDescriptorIndex;
}

static void doReadDirectoryEntry(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	bool endOfDirectory;
	processExecutionState2->eax = ioServicesReadDirectoryEntry(currentProcess, processExecutionState2->ebx, (void*) processExecutionState2->ecx, true, &endOfDirectory);
	processExecutionState2->ebx = endOfDirectory;
}

static void doRead(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesRead(currentProcess, processExecutionState2->ebx, true,
		(void*) processExecutionState2->ecx, (size_t) processExecutionState2->edx, &processExecutionState2->ebx);
}

static void doChangeFileSize(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesChangeFileSize(currentProcess, processExecutionState2->ebx, processExecutionState2->ecx);
}

static void doCreateDirectory(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	const char* newDirectoryPath = (const char*) processExecutionState2->ebx;
	mode_t mode = processExecutionState2->ecx;

	processExecutionState2->eax = ioServicesCreateDirectory(currentProcess, newDirectoryPath, true, mode);
}

static void doWrite(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesWrite(currentProcess, processExecutionState2->ebx, true,
		(void*) processExecutionState2->ecx, (size_t) processExecutionState2->edx, &processExecutionState2->ebx);
}

static void doClose(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesClose(currentProcess, processExecutionState2->ebx);
}

static void doStatus(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	int fileDescriptorIndex = processExecutionState2->ebx;
	struct stat* statInstance = (struct stat*) processExecutionState2->ecx;

	processExecutionState2->eax = ioServicesStatus(currentProcess, fileDescriptorIndex, true, statInstance);
}

static void doRepositionOpenFileDescriptionOffset(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;

	off_t newOffset;
	APIStatusCode result = ioServicesRepositionOpenFileDescriptionOffset(currentProcess, processExecutionState2->ebx,
		(off_t) processExecutionState2->ecx, (int) processExecutionState2->edx, &newOffset);
	if (result == SUCCESS) {
		processExecutionState2->ebx = newOffset;
	}

	processExecutionState2->eax = result;
}

static void doGetCurrentWorkingDirectory(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processGetCurrentWorkingDirectory(currentProcess, (char*) processExecutionState2->ebx, processExecutionState2->ecx);
}

static void doSetCurrentWorkingDirectory(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesSetCurrentWorkingDirectory(currentProcess, (char*) processExecutionState2->ebx);
}

static void doChangeDataSegmentSize(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processManagerChangeDataSegmentSize(currentProcess, processExecutionState2->ebx);
	processExecutionState2->ebx = memoryManagerCalculateProcessFirstInvalidDataSegmentAddress(currentProcess);
}

static void doLogKernelModuleDebugReport(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = debugSystemCallServicesLogKernelModuleDebugReport(currentProcess, (void*) processExecutionState2->ebx);
}

static void doReleaseName(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesReleaseName(currentProcess, (void*) processExecutionState2->ebx, true);
}

static void doReleaseDirectory(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesReleaseDirectory(currentProcess, (char*) processExecutionState2->ebx, true);
}

static void doCreateSymbolicLink(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesCreateSymbolicLink(currentProcess, (char*) processExecutionState2->ebx, (char*) processExecutionState2->ecx, true);
}

static void doChangeDeviceParameters(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesChangeDeviceParameters(currentProcess, processExecutionState2->ebx, (void*) processExecutionState2->ecx, true);
}

static void doChangeOpenFileDescriptionParameters(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesChangeOpenFileDescriptionParameters(currentProcess, processExecutionState2->ebx, (void*) processExecutionState2->ecx, true);
}

static void doChangeFileDescriptorParameters(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesChangeFileDescriptorParameters(currentProcess, processExecutionState2->ebx, (void*) processExecutionState2->ecx, true);
}

static void doDuplicateFileDescriptor(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesDuplicateFileDescriptor(currentProcess, processExecutionState2->ebx, processExecutionState2->ecx,
		(int*) processExecutionState2->edx, true);
}

static void doCreateSessionAndProcessGroup(struct Process* currentProcess) {
	APIStatusCode result = processServicesCreateSessionAndProcessGroup(currentProcess);
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = result;
	processExecutionState2->ebx = currentProcess->id;
}

static void doGetSessionId(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processGetSessionId(currentProcess, processExecutionState2->ebx, (pid_t*) &processExecutionState2->ebx);
}

static void doGetProcessGroupId(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processServicesGetProcessGroupId(currentProcess, processExecutionState2->ebx, (pid_t*) &processExecutionState2->ebx);
}

static void doSetProcessGroup(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processServicesSetProcessGroup(currentProcess, processExecutionState2->ebx, processExecutionState2->ecx);
}

static void doCreatePipe(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = pipeManagerCreatePipe(currentProcess, (int*) &processExecutionState2->ebx, (int*) &processExecutionState2->ecx);
}

static void doMonitorIOEvents(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = ioServicesMonitorIOEvents(currentProcess, (struct pollfd*) processExecutionState2->ebx, (nfds_t) processExecutionState2->ecx, (int) processExecutionState2->edx, (int*) &processExecutionState2->ebx);
}

static void doGetProcessMemorySegmentsLimits(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processExecutionState2->eax = processGetProcessMemorySegmentsLimits(currentProcess, (void*) processExecutionState2->ebx, true);
}

static void doSleep(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	processServicesSleep(currentProcess, processExecutionState2->ebx);
}

static void doSetFileModeCreationMask(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	mode_t newFileModeCreationMask = processExecutionState2->ebx & 0777;
	processExecutionState2->eax = currentProcess->fileModeCreationMask;
	currentProcess->fileModeCreationMask = newFileModeCreationMask;
}

static void doRename(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	const char* oldPath = (void*) processExecutionState2->ebx;
	const char* newPath = (void*) processExecutionState2->ecx;
	processExecutionState2->eax = ioServicesRename(currentProcess, oldPath, newPath, true);
}

static void doForkAndGenerateSignal(struct Process* currentProcess) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	struct Process* childProcess;
	int signalId = processExecutionState2->ebx;
	APIStatusCode result = processManagerForkProcess(currentProcess, &childProcess);
	if (result == SUCCESS) {
		if (signalId > 0) {
			signalServicesGenerateSignal(currentProcess, childProcess->id, signalId, false, NULL);
		}
		processExecutionState2->ebx = childProcess->id;
	}
	processExecutionState2->eax = result;
}

static void systemCallHandler(struct ProcessExecutionState1* processExecutionState1, struct ProcessExecutionState2* processExecutionState2) {
	struct Process* currentProcess = processManagerGetCurrentProcess();
	assert(currentProcess != NULL);
	assert(currentProcess->state == RUNNABLE);

	currentProcess->processExecutionState1 = processExecutionState1;
	currentProcess->processExecutionState2 = processExecutionState2;

	int systemCallId = processExecutionState2->eax;
	switch (systemCallId) {
		case SYSTEM_CALL_WAIT:
			doWait(currentProcess);
			break;

		case SYSTEM_CALL_GET_PROCESS_ID:
			processExecutionState2->eax = currentProcess->id;
			break;

		case SYSTEM_CALL_GET_PARENT_PROCESS_ID:
			{
				pid_t parentProcessId = 0;
				if (currentProcess->parentProcess != NULL) {
					parentProcessId = currentProcess->parentProcess->id;
				}
				processExecutionState2->eax = parentProcessId;
			}
			break;

		case SYSTEM_CALL_SLEEP:
			doSleep(currentProcess);
			break;

		case SYSTEM_CALL_EXIT:
			{
				assert(currentProcess->state == RUNNABLE);
				int exitStatus = WIFEXITED_MASK | (WEXITSTATUS_MASK & (processExecutionState2->ebx << WEXITSTATUS_SHIFT));
				processManagerTerminate(currentProcess, exitStatus, 0);
				processManagerScheduleProcessExecution();
			}
			break;

		case SYSTEM_CALL_FORK:
			doForkAndGenerateSignal(currentProcess);
			break;

		case SYSTEM_CALL_OPEN:
			doOpen(currentProcess);
			break;

		case SYSTEM_CALL_CHANGE_DEVICE_PARAMETERS:
			doChangeDeviceParameters(currentProcess);
			break;

		case SYSTEM_CALL_WRITE:
			doWrite(currentProcess);
			break;

		case SYSTEM_CALL_READ:
			doRead(currentProcess);
			break;

		case SYSTEM_CALL_CLOSE:
			doClose(currentProcess);
			break;

		case SYSTEM_CALL_CHANGE_DATA_SEGMENT_SIZE:
			doChangeDataSegmentSize(currentProcess);
			break;

		case SYSTEM_CALL_GET_UNIX_TIME:
			{
				/*
				 * There are better ways to implement this system call.
				 * However, there are things with higher priority to be implemented before.
				 */
				processExecutionState2->eax = cmosGetUnixTime();
			}
			break;

		case SYSTEM_CALL_CREATE_SYMBOLIC_LINK:
			doCreateSymbolicLink(currentProcess);
			break;

		case SYSTEM_CALL_STATUS:
			doStatus(currentProcess);
			break;

		case SYSTEM_CALL_READ_DIRECTORY_ENTRY:
			doReadDirectoryEntry(currentProcess);
			break;

		case SYSTEM_CALL_EXECUTE_EXECUTABLE:
			doExecuteExecutable(currentProcess);
			break;

		case SYSTEM_CALL_REPOSITION_OPEN_FILE_DESCRIPTION_OFFSET:
			doRepositionOpenFileDescriptionOffset(currentProcess);
			break;

		case SYSTEM_CALL_CHANGE_SIGNAL_ACTION:
			doChangeSignalAction(currentProcess);
			break;

		case SYSTEM_CALL_GET_CURRENT_WORKING_DIRECTORY:
			doGetCurrentWorkingDirectory(currentProcess);
			break;

		case SYSTEM_CALL_SET_CURRENT_WORKING_DIRECTORY:
			doSetCurrentWorkingDirectory(currentProcess);
			break;

		case SYSTEM_CALL_RELEASE_NAME:
			doReleaseName(currentProcess);
			break;

		case SYSTEM_CALL_GENERATE_SIGNAL:
			doGenerateSignal(currentProcess);
			break;

		case SYSTEM_CALL_CREATE_NAME:
			doCreateName(currentProcess);
			break;

		case SYSTEM_CALL_REBOOT: {
			doReboot(currentProcess);
		} break;

		case SYSTEM_CALL_CACHE_FLUSH:
			blockCacheManageFlush();
			break;

		case SYSTEM_CALL_CACHE_FLUSH_AND_CLEAR:
			blockCacheManageFlush();
			blockCacheManageClear();
			break;

		case SYSTEM_CALL_CHANGE_SIGNALS_BLOCKAGE:
			doChangeSignalBlockage(currentProcess);
			break;

		case SYSTEM_CALL_RELEASE_DIRECTORY:
			doReleaseDirectory(currentProcess);
			break;

		case SYSTEM_CALL_CHANGE_FILE_SIZE:
			doChangeFileSize(currentProcess);
			break;

		case SYSTEM_CALL_CREATE_DIRECTORY:
			doCreateDirectory(currentProcess);
			break;

		case SYSTEM_CALL_CHANGE_OPEN_FILE_DESCRIPTION_PARAMETERS:
			doChangeOpenFileDescriptionParameters(currentProcess);
			break;

		case SYSTEM_CALL_DUPLICATE_FILE_DESCRIPTOR:
			doDuplicateFileDescriptor(currentProcess);
			break;

		case SYSTEM_CALL_CREATE_SESSION_AND_PROCESS_GROUP:
			doCreateSessionAndProcessGroup(currentProcess);
			break;

		case SYSTEM_CALL_GET_SESSION_ID:
			doGetSessionId(currentProcess);
			break;

		case SYSTEM_CALL_GET_PROCESS_GROUP_ID:
			doGetProcessGroupId(currentProcess);
			break;

		case SYSTEM_CALL_SET_PROCESS_GROUP:
			doSetProcessGroup(currentProcess);
			break;

		case SYSTEM_CALL_CREATE_PIPE:
			doCreatePipe(currentProcess);
			break;

		case SYSTEM_CALL_MONITOR_IO_EVENTS:
			doMonitorIOEvents(currentProcess);
			break;

		case SYSTEM_CALL_SET_FILE_MODE_CREATION_MASK:
			doSetFileModeCreationMask(currentProcess);
			break;

		case SYSTEM_CALL_RENAME:
			doRename(currentProcess);
			break;

		case SYSTEM_CALL_CHANGE_FILE_DESCRIPTOR_PARAMETERS:
			doChangeFileDescriptorParameters(currentProcess);
			break;

		/*
		 * Debug system calls:
		 */
		case SYSTEM_CALL_BUSY_WAIT:
			busyWaitingSleep(5000);
			break;

		case SYSTEM_CALL_ASSERT_FALSE:
			{
				logDebug("The process which id is %d called \"SYSTEM_CALL_ASSERT_FALSE\"", currentProcess->id);
				assert(false);
			}
			break;

		case SYSTEM_CALL_LOG_KERNEL_MODULE_DEBUG_REPORT:
			doLogKernelModuleDebugReport(currentProcess);
			break;

		case SYSTEM_CALL_FORCE_KERNEL_SIGSEGV:
			{
				/* Load an invalid segment selector. */
				__asm__ __volatile__(
					"mov $0xFFF8, %%cx;"
					"mov %%cx, %%gs;"
					:
					:
					: "ecx", "memory");
			}
			break;

		case SYSTEM_CALL_FORCE_KERNEL_SIGILL:
			{
				int number = -1;
				void (*invalidOpcode)() = (void(*)()) &number;
				invalidOpcode();
			}
			break;

		case SYSTEM_CALL_GET_PROCESS_MEMORY_SEGMENTS_LIMITS:
			doGetProcessMemorySegmentsLimits(currentProcess);
			break;

		default:
			logDebug("The process whose id is %d called an inexistent system call (%d)", currentProcess->id, systemCallId);
			signalServicesGenerateSignal(currentProcess, currentProcess->id, SIGSYS, false, NULL);
			break;
	}
}

void systemCallManagerInitialize(void) {
	interruptionManagerRegisterSystemCallHandler(&systemCallHandler);
}
