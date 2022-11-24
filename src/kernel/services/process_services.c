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

#include <ctype.h>
#include <fcntl.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "kernel/command_scheduler.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/session_manager.h"

#include "kernel/io/open_file_description.h"

#include "kernel/process/process_group.h"
#include "kernel/process/process_group_manager.h"
#include "kernel/process/process_manager.h"

#include "kernel/services/io_services.h"
#include "kernel/services/process_services.h"
#include "kernel/services/signal_services.h"

#include "standard_library_implementation/file_descriptor_offset_reposition_constants.h"

#include "util/string_stream_writer.h"

void processServicesSuspendToWaitForIO(struct Process* currentProcess, struct DoubleLinkedList* list, enum ProcessState newState) {
	assert(currentProcess->state == RUNNABLE);
	assert(currentProcess->waitingIOProcessList == NULL);

	if (list != NULL) {
		assert(!doubleLinkedListContainsFoward(list, &currentProcess->waitingIOProcessListElement));
		doubleLinkedListInsertAfterLast(list, &currentProcess->waitingIOProcessListElement);
		currentProcess->waitingIOProcessList = list;

	} else {
		assert(currentProcess->usedIOEventMonitoringContextsCount > 0);
		assert(processCountIOEventsBeingMonitored(currentProcess) > 0);
	}

	assert(newState == SUSPENDED_WAITING_READ || newState == SUSPENDED_WAITING_WRITE || newState == SUSPENDED_WAITING_IO_EVENT);
	processManagerChangeProcessState(currentProcess, currentProcess, newState, 0);
}

enum ExecutableFormat {
	EXECUTABLE_FORMAT_ELF,
	EXECUTABLE_FORMAT_BINARY,
	EXECUTABLE_FORMAT_SCRIPT,
	EXECUTABLE_FORMAT_UNKNOWN
};

struct ExecuteExecutableContext {
	struct DoubleLinkedList pageFramesToReleaseList;

	void* executableFirstBytesBuffer;
	size_t executableFirstBytesCount;

	char* scriptInterpreterPath;
	const char** arguments;
	int argumentCount;
	const char** environmentParameters;
	int environmentParameteCount;

	struct StringStreamWriter argumentsWriter;
	struct StringStreamWriter environmentParametersWriter;

	struct Process* process;
};

const char* const SCRIPT_FIRST_BYTES = "#!";
const int SCRIPT_FIRST_BYTES_COUNT = 2;

enum ExecutableFormat determineExecutableFormat(struct ExecuteExecutableContext* executeExecutableContext) {
	APIStatusCode executableFormat = EXECUTABLE_FORMAT_UNKNOWN;

	if (executeExecutableContext->executableFirstBytesCount > 0) {
		executableFormat = EXECUTABLE_FORMAT_BINARY;

		if (executeExecutableContext->executableFirstBytesCount > SCRIPT_FIRST_BYTES_COUNT &&
				strncmp(SCRIPT_FIRST_BYTES, (char*) executeExecutableContext->executableFirstBytesBuffer, SCRIPT_FIRST_BYTES_COUNT) == 0) {
			executableFormat = EXECUTABLE_FORMAT_SCRIPT;
		}
	}

	return executableFormat;
}

#define MAX_ARGUMENTS ((PAGE_FRAME_SIZE - PATH_MAX_LENGTH) / 2 / sizeof(char*))
#define MAX_ENVIRONMENT_PARAMETERS MAX_ARGUMENTS
_Static_assert(PATH_MAX_LENGTH
		+ MAX_ARGUMENTS * sizeof(char*)
		+ MAX_ENVIRONMENT_PARAMETERS * sizeof(char*) == PAGE_FRAME_SIZE, "Expecting sum to be equals to PAGE_FRAME_SIZE.");

static APIStatusCode initializeExecuteExecutableContext(struct ExecuteExecutableContext* executeExecutableContext, struct Process* process) {
	APIStatusCode result = SUCCESS;

	memset(executeExecutableContext, 0, sizeof(struct ExecuteExecutableContext));
	doubleLinkedListInitialize(&executeExecutableContext->pageFramesToReleaseList);

	struct DoubleLinkedListElement* doubleLinkedListElement;
	doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		executeExecutableContext->executableFirstBytesBuffer = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		doubleLinkedListInsertAfterLast(&executeExecutableContext->pageFramesToReleaseList, doubleLinkedListElement);
	} else {
		result = ENOMEM;
	}

	doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		executeExecutableContext->scriptInterpreterPath = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		executeExecutableContext->arguments = (void*) executeExecutableContext->scriptInterpreterPath + PATH_MAX_LENGTH;
		executeExecutableContext->environmentParameters = (void*) executeExecutableContext->arguments + sizeof(char*) * MAX_ARGUMENTS;
		assert((void*) executeExecutableContext->scriptInterpreterPath + PAGE_FRAME_SIZE ==
				(void*) executeExecutableContext->environmentParameters + sizeof(char*) * MAX_ENVIRONMENT_PARAMETERS);
		doubleLinkedListInsertAfterLast(&executeExecutableContext->pageFramesToReleaseList, doubleLinkedListElement);
	} else {
		result = ENOMEM;
	}

	doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		char* buffer = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		stringStreamWriterInitialize(&executeExecutableContext->argumentsWriter, buffer, PAGE_FRAME_SIZE);
		doubleLinkedListInsertAfterLast(&executeExecutableContext->pageFramesToReleaseList, doubleLinkedListElement);
	} else {
		result = ENOMEM;
	}

	doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		char* buffer = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		stringStreamWriterInitialize(&executeExecutableContext->environmentParametersWriter, buffer, PAGE_FRAME_SIZE);
		doubleLinkedListInsertAfterLast(&executeExecutableContext->pageFramesToReleaseList, doubleLinkedListElement);
	} else {
		result = ENOMEM;
	}

	executeExecutableContext->process = process;

	return result;
}

static APIStatusCode pushArgumentOrEnvironmentParameter(struct ExecuteExecutableContext* executeExecutableContext,
		const char* item, bool isArgument, bool verifyUserAddress) {
	APIStatusCode result = SUCCESS;

	if (verifyUserAddress && !processIsValidSegmentAccess(executeExecutableContext->process, (uint32_t) item, sizeof(char))) {
		result = EFAULT;

	} else {
		struct StringStreamWriter* stringStreamWriter;
		const char** items;
		int* itemCount;
		int maximumItems;
		if (isArgument) {
			stringStreamWriter = &executeExecutableContext->argumentsWriter;
			itemCount = &executeExecutableContext->argumentCount;
			maximumItems = MAX_ARGUMENTS;
			items = executeExecutableContext->arguments;
		} else {
			stringStreamWriter = &executeExecutableContext->environmentParametersWriter;
			itemCount = &executeExecutableContext->environmentParameteCount;
			maximumItems = MAX_ENVIRONMENT_PARAMETERS;
			items = executeExecutableContext->environmentParameters;
		}

		if (*itemCount + 1 > maximumItems) {
			result = E2BIG;
		} else {
			items[*itemCount] = stringStreamWriterNextCharacterPointer(stringStreamWriter);
			streamWriterWriteString(&stringStreamWriter->streamWriter, item, UINT_MAX);
			streamWriterWriteCharacter(&stringStreamWriter->streamWriter, '\0');
			if (!streamWriterMayAcceptMoreData(&stringStreamWriter->streamWriter)) {
				result = E2BIG;
			}
			(*itemCount)++;
		}

		if (result == SUCCESS && streamWriterGetWrittenCharacterCount(&executeExecutableContext->argumentsWriter.streamWriter)
				+ streamWriterGetWrittenCharacterCount(&executeExecutableContext->environmentParametersWriter.streamWriter) > ARG_MAX) {
			result = E2BIG;
		}
	}

	return result;
}

static void invertOrderOfLastPushedArguments(struct ExecuteExecutableContext* executeExecutableContext, int pushedArgumentCount) {
	const char** arguments = executeExecutableContext->arguments;
	int argumentsCount = executeExecutableContext->argumentCount;
	for (int i = 0; i < pushedArgumentCount / 2; i++) {

		const char* aux = arguments[argumentsCount - pushedArgumentCount + i];
		arguments[argumentsCount - pushedArgumentCount + i] = arguments[argumentsCount - 1 - i];
		arguments[argumentsCount - 1 - i] = aux;
	}
}

static APIStatusCode prepareScriptExecution(struct ExecuteExecutableContext* executeExecutableContext) {
	APIStatusCode result = SUCCESS;

	char* firstLine = executeExecutableContext->executableFirstBytesBuffer;

	int pushedArgumentCount = 0;
	char* currentArgument = NULL;
	bool foundScriptInterpreterPath = false;
	char* scriptInterpreterPath = NULL;
	for (int i = SCRIPT_FIRST_BYTES_COUNT; i < executeExecutableContext->executableFirstBytesCount && result == SUCCESS; i++) {
		char c = firstLine[i];

		if (isspace(c)) {
			if (!foundScriptInterpreterPath) {
				firstLine[i] = '\0';

				size_t scriptInterpreterPathLength = i - SCRIPT_FIRST_BYTES_COUNT;

				if (scriptInterpreterPathLength == 0) {
					result = ENOEXEC;
				} else if (scriptInterpreterPathLength >= PATH_MAX_LENGTH) {
					result = ENAMETOOLONG;
				} else {
					strcpy(executeExecutableContext->scriptInterpreterPath, scriptInterpreterPath);
					foundScriptInterpreterPath = true;
				}

			} else if (currentArgument != NULL) {
				firstLine[i] = '\0';
				result = pushArgumentOrEnvironmentParameter(executeExecutableContext, currentArgument, true, false);
				pushedArgumentCount++;
				currentArgument = NULL;
			}

			if (c == '\n' || c == '\r') {
				break;
			}

		} else if (foundScriptInterpreterPath && currentArgument == NULL) {
			currentArgument = &firstLine[i];

		} else if (!foundScriptInterpreterPath && scriptInterpreterPath == NULL) {
			scriptInterpreterPath = &firstLine[i];
		}
	}

	if (!foundScriptInterpreterPath) {
		result = ENOEXEC;
	}

	if (result == SUCCESS) {
		invertOrderOfLastPushedArguments(executeExecutableContext, pushedArgumentCount);
	}

	return result;
}

static void realeaseExecuteExecutableContext(struct ExecuteExecutableContext* executeExecutableContext) {
	struct DoubleLinkedListElement* element = doubleLinkedListFirst(&executeExecutableContext->pageFramesToReleaseList);
	while (element != NULL) {
		struct DoubleLinkedListElement* next = element->next;
		memoryManagerReleasePageFrame(element, -1);
		element = next;
	}
}

APIStatusCode processServicesExecuteExecutable(struct Process* currentProcess, bool verifyUserAddress, const char* executablePath, const char** argv, const char** envp) {
	APIStatusCode result = SUCCESS;

	if (verifyUserAddress && (!processIsValidSegmentAccess(currentProcess, (uint32_t) executablePath, sizeof(char*))
			|| (argv != NULL && !processIsValidSegmentAccess(currentProcess, (uint32_t) argv, sizeof(char**)))
			|| (envp != NULL && !processIsValidSegmentAccess(currentProcess, (uint32_t) envp, sizeof(char**))))) {
		result = EFAULT;

	} else {
		struct ExecuteExecutableContext executeExecutableContext;
		result = initializeExecuteExecutableContext(&executeExecutableContext, currentProcess);

		/* Push "argv" and "envp". */
		if (result == SUCCESS) {
			if (argv != NULL) {
				int argumentsCount = 0;
				while (argv[argumentsCount] != NULL) {
					argumentsCount++;
				}

				for (int i = argumentsCount - 1; i >= 0 && result == SUCCESS; i--) {
					result = pushArgumentOrEnvironmentParameter(&executeExecutableContext, argv[i], true, verifyUserAddress);
				}
				assert(result != SUCCESS || argumentsCount == executeExecutableContext.argumentCount);
			}
			if (envp != NULL) {
				int environmentParametersCount;
				for (environmentParametersCount = 0; envp[environmentParametersCount] != NULL && result == SUCCESS; environmentParametersCount++) {
					result = pushArgumentOrEnvironmentParameter(&executeExecutableContext, envp[environmentParametersCount], false, verifyUserAddress);
				}
				assert(result != SUCCESS || environmentParametersCount == executeExecutableContext.environmentParameteCount);
			}
		}

		/* Prepare the executable. */
		if (result == SUCCESS) {
			enum ExecutableFormat executableFormat;
			#define MAX_RECURSION_DEPTH 5
			int depth = 0;
			bool stop = false;

			do {
				int flags = O_RDONLY;
				int fileDescriptorIndex;
				/* It opens the file that contains the executable to read it to the memory. */
				result = ioServicesOpen(currentProcess, false, executablePath, false, flags, 0, &fileDescriptorIndex);

				if (result == SUCCESS) {
					result = ioServicesRead(currentProcess, fileDescriptorIndex, false, executeExecutableContext.executableFirstBytesBuffer, PAGE_FRAME_SIZE,
						&executeExecutableContext.executableFirstBytesCount);

					if (result == SUCCESS) {
						executableFormat = determineExecutableFormat(&executeExecutableContext);

						if (executableFormat == EXECUTABLE_FORMAT_SCRIPT) {
							depth++;
							if (depth > MAX_RECURSION_DEPTH) {
								result = ELOOP;

							} else {
								result = prepareScriptExecution(&executeExecutableContext);
								if (result == SUCCESS) {
									executablePath = executeExecutableContext.scriptInterpreterPath;
									result = pushArgumentOrEnvironmentParameter(&executeExecutableContext, executablePath, true, false);
								}
							}

						} else if (executableFormat == EXECUTABLE_FORMAT_BINARY) {
							struct stat statInstance;
							result = ioServicesStatus(currentProcess, fileDescriptorIndex, false, &statInstance);
							if (result == SUCCESS) {
								size_t executableSize = statInstance.st_size;

								if (S_ISREG(statInstance.st_mode)) {
									if (executableSize == 0 || executableSize > EXECUTABLE_MAX_SIZE) {
										result = ENOEXEC;
									} else {
										result = processManagerChangeCodeSegmentSize(currentProcess, executableSize, false);
									}

								} else {
									result = EACCES;
								}

								if (result == SUCCESS) {
									off_t newOffset;
									result = ioServicesRepositionOpenFileDescriptionOffset(currentProcess, fileDescriptorIndex, 0, SEEK_SET, &newOffset);

									if (result == SUCCESS) {
										/* It copies the program properly. */
										size_t count = 0;
										result = ioServicesRead(currentProcess, fileDescriptorIndex, false, (void*) CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS, executableSize, &count);
										assert(result != SUCCESS || count == executableSize);

										/* It releases the data segment entirely as it already copied the "argv" and "envp". */
										if (result == SUCCESS) {
											processManagerChangeDataSegmentSize(currentProcess, -doubleLinkedListSize(&currentProcess->dataSegmentPageFramesList) * PAGE_FRAME_SIZE);
											assert(doubleLinkedListSize(&currentProcess->dataSegmentPageFramesList) == 0);
										}
									}
								}

								if (result == SUCCESS) {
									result = processManagerChangeCodeSegmentSize(currentProcess, executableSize, true);
									assert(result == SUCCESS); /* Because if there is something to do it would be a segment reduction. */
								}
							}
							stop = true;

						} else {
							result = ENOEXEC;
						}
					}

					ioServicesClose(currentProcess, fileDescriptorIndex);
				}

			} while (!stop && result == SUCCESS);
		}

		if (result == SUCCESS) {
			/* From now on, a failure can not happen. */

			currentProcess->executedExecutableAfterFork = true;

			/*
			 * Directory streams open in the calling process image shall be closed in the new process image.
			 * File descriptors that are marked close-on-exec are closed too.
			 *
			 * https://man7.org/linux/man-pages/man2/execve.2.html
			 */
			for (int i = 0; i < MAX_FILE_DESCRIPTORS_PER_PROCESS; i++) {
				struct FileDescriptor* fileDescriptor = &currentProcess->fileDescriptors[i];
				struct OpenFileDescription* openFileDescription = fileDescriptor->openFileDescription;

				if (openFileDescription != NULL) {
					struct VirtualFileSystemOperations* operations = openFileDescription->virtualFileSystemNode->operations;
					assert(operations->getMode != NULL);
					mode_t mode = operations->getMode(openFileDescription->virtualFileSystemNode);
					if (S_ISDIR(mode) || fileDescriptor->flags == FD_CLOEXEC) {
						ioServicesClose(currentProcess, i);
					}
				}
			}

			/*
			 * The dispositions of any signals that are being caught are reset to the default (signal(7)).
			 *
			 * https://man7.org/linux/man-pages/man2/execve.2.html
			 */
			for (int signalId = 1; signalId <= NUMBER_OF_SIGNALS; signalId++) {
				struct SignalInformation* signalInformation = &currentProcess->signalInformation[signalId - 1];
				if (signalInformation->callback != NULL) {
					assert(!signalInformation->ignored);
					signalInformation->callback = NULL;
				}
			}

			/*
			 * Prepare to copy "argv" and "envp".
			 */
			size_t argumentsAndEnvironmentParametersSize = (executeExecutableContext.argumentCount + 1) * sizeof(char*)
					+ streamWriterGetWrittenCharacterCount(&executeExecutableContext.argumentsWriter.streamWriter)
					+ (executeExecutableContext.environmentParameteCount + 1) * sizeof(char*)
					+ streamWriterGetWrittenCharacterCount(&executeExecutableContext.environmentParametersWriter.streamWriter);
			if (argumentsAndEnvironmentParametersSize % 4 != 0) {
				argumentsAndEnvironmentParametersSize += 4 - (argumentsAndEnvironmentParametersSize % 4);
			}

			uint32_t esp3 = STACK_SEGMENT_FIRST_INVALID_VIRTUAL_ADDRESS_AFTER - argumentsAndEnvironmentParametersSize;
			assert(esp3 % 4 == 0);
			currentProcess->processExecutionState1->esp3 = esp3 - sizeof(uint32_t);

			char** argvCopy = (char**) esp3;
			char** envpCopy = (char**) (esp3 + sizeof(char*) * (executeExecutableContext.argumentCount + 1));

			char* argumentsAndEnvironmentParameters = ((void*) envpCopy) + sizeof(char*) * (executeExecutableContext.environmentParameteCount + 1);

			/* Copy "argv" and "envp" to final destination. */
			for (int i = executeExecutableContext.argumentCount - 1; i >= 0; i--) {
				size_t length = strlen(executeExecutableContext.arguments[i]);
				strcpy(argumentsAndEnvironmentParameters, executeExecutableContext.arguments[i]);
				argvCopy[executeExecutableContext.argumentCount - 1 - i] = argumentsAndEnvironmentParameters;
				argumentsAndEnvironmentParameters += length + 1;
			}
			argvCopy[executeExecutableContext.argumentCount] = NULL;

			for (int i = 0; i < executeExecutableContext.environmentParameteCount; i++) {
				size_t length = strlen(executeExecutableContext.environmentParameters[i]);
				strcpy(argumentsAndEnvironmentParameters, executeExecutableContext.environmentParameters[i]);
				envpCopy[i] = argumentsAndEnvironmentParameters;
				argumentsAndEnvironmentParameters += length + 1;
			}
			envpCopy[executeExecutableContext.environmentParameteCount] = NULL;

			currentProcess->fpuInitialized = false;

			/* It adjusts the process' state. */
			currentProcess->processExecutionState1->eip = CODE_SEGMENT_FIRST_PAGE_VIRTUAL_ADDRESS;
			currentProcess->processExecutionState2->ebp = 0;
			currentProcess->processExecutionState2->eax = executeExecutableContext.argumentCount;
			currentProcess->processExecutionState2->ebx = (uint32_t) argvCopy;
			currentProcess->processExecutionState2->ecx = (uint32_t) envpCopy;
			currentProcess->processExecutionState2->edx = 0;
			currentProcess->processExecutionState2->edi = 0;
			currentProcess->processExecutionState2->esi = 0;
		}

		realeaseExecuteExecutableContext(&executeExecutableContext);
	}

	return result;
}

APIStatusCode processServicesCreateSessionAndProcessGroup(struct Process* leaderProcess) {
	APIStatusCode result = SUCCESS;

	struct ProcessGroup* processGroup = processGroupManagerGetAndReserveProcessGroupById(leaderProcess->id);
	if (processGroup == NULL) {
		/* Is it possible to create a new session and a new process group? */
		if (sessionManagerHasResourcesForNewOne() && processGroupManagerHasResourcesForNewOne()) {
			processGroupRemoveProcess(leaderProcess);

			struct Session* session = sessionManagerAcquireSession(leaderProcess);
			assert(session != NULL);
			processGroup = processGroupManagerAcquireProcessGroup(leaderProcess);
			assert(processGroup != NULL);
			sessionManagerInsertProcessGroup(session, processGroup);

			sessionManagerReleaseSession(session);

		} else {
			result = ENOMEM;
		}

	} else if (processGroup->id != leaderProcess->id || processGroup->session->id != leaderProcess->id) {
		/* It fails if the calling process is already a process group leader and if the new session is different from the current one. */
		result = EPERM;
	}

	if (processGroup != NULL) {
		processGroupManagerReleaseReservation(processGroup);
	}

	return result;
}

APIStatusCode processGetSessionId(struct Process* currentProcess, pid_t processId, pid_t* sessionId) {
	APIStatusCode result = SUCCESS;
	struct Process* targetProcess = NULL;
	struct Session* session = NULL;

	if (processId <= 0) {
		targetProcess = currentProcess;
	} else {
		targetProcess = processManagerGetProcessById(processId);
	}

	if (targetProcess != NULL) {
		session = processGetSession(targetProcess);
	} else {
		result = ESRCH;
	}

	if (session != NULL) {
		*sessionId = session->id;
	} else {
		*sessionId = -1;
	}

	return result;
}

APIStatusCode processServicesGetProcessGroupId(struct Process* currentProcess, pid_t processId, pid_t* processGroupId) {
	APIStatusCode result = SUCCESS;
	struct Process* targetProcess = NULL;
	struct ProcessGroup* processGroup = NULL;

	if (processId <= 0) {
		targetProcess = currentProcess;
	} else {
		targetProcess = processManagerGetProcessById(processId);
	}

	if (targetProcess != NULL) {
		processGroup = targetProcess->processGroup;
	} else {
		result = ESRCH;
	}

	if (processGroup != NULL) {
		*processGroupId = processGroup->id;
	} else {
		*processGroupId = -1;
	}

	return result;
}

static bool shouldChildCauseWaitToReturn(struct Process* currentProcess, struct Process* childProcess, int options, int* status, pid_t* childProcessId) {
	assert(childProcess != NULL && childProcess->parentProcess == currentProcess);

	bool result = false;

	if (childProcess->state == WAITING_EXIT_STATUS_COLLECTION) {
		*status = childProcess->exitStatus;
		processManagerReleaseProcessResources(childProcess);
		result = true;

	} else if ((options & WCONTINUED) != 0 && childProcess->stateToNotifyParentAbout == RUNNABLE) {
		*status = WIFCONTINUED_MASK;
		result = true;

	} else if ((options & WUNTRACED) != 0 && childProcess->stateToNotifyParentAbout == STOPPED) {
		*status = WIFSTOPPED_MASK | (WSTOPSIG_MASK & (childProcess->signalToNotifyProcessAbout  << WSTOPSIG_SHIFT));
		result = true;
	}

	if (result) {
		childProcess->stateToNotifyParentAbout = ABSENT;
		childProcess->signalToNotifyProcessAbout = 0;
		*childProcessId = childProcess->id;
	}

	return result;
}

APIStatusCode processServicesWait(struct Process* currentProcess, pid_t scope, int options, int* status, pid_t* childProcessId) {
	APIStatusCode result = SUCCESS;

	/* This process has any child? */
	bool done = false;

	do {
		int childrenCount = 0;
		pid_t processGroupId = 0;
		if (scope > 0) {
			struct Process* process = processManagerGetProcessById(scope);
			if (process != NULL && process->parentProcess == currentProcess) {
				childrenCount++;
			}

		} else if (scope == -1) {
			childrenCount = doubleLinkedListSize(&currentProcess->childrenProcessList);

		} else {
			if (scope == 0) {
				/* Any child process whose process group ID is equal to that of the calling process. */
				processGroupId = currentProcess->id;

			} else {
				/* Any child process whose process group ID is equal to the absolute value of scope. */
				assert(scope < 0);
				processGroupId = -scope;
			}

			struct ProcessGroup* processGroup = processGroupManagerGetAndReserveProcessGroupById(processGroupId);
			if (processGroup != NULL) {
				struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&processGroup->processesList);
				while (doubleLinkedListElement != NULL) {
					struct Process* process = processGetProcessFromProcessGroupListElement(doubleLinkedListElement);
					doubleLinkedListElement = doubleLinkedListElement->next;

					if (process->parentProcess == currentProcess) {
						childrenCount++;
					}
				}
			}
			processGroupManagerReleaseReservation(processGroup);
		}

		if (childrenCount > 0) {
			if (scope == -1) {
				struct Process* childProcess = NULL;
				struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&currentProcess->childrenProcessList);

				while (listElement != NULL && !done) {
					childProcess = processGetProcessFromChildrenProcessListElement(listElement);
					listElement = listElement->next;
					done = shouldChildCauseWaitToReturn(currentProcess, childProcess, options, status, childProcessId);
				}

			} else if (scope > 0) {
				struct Process* childProcess = processManagerGetProcessById(scope);
				done = shouldChildCauseWaitToReturn(currentProcess, childProcess, options, status, childProcessId);

			} else {
				assert(processGroupId > 0);

				struct ProcessGroup* processGroup = processGroupManagerGetAndReserveProcessGroupById(processGroupId);
				assert(processGroup != NULL);

				struct Process* childProcess = NULL;
				struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&processGroup->processesList);
				while (listElement != NULL && !done) {
					childProcess = processGetProcessFromProcessGroupListElement(listElement);
					listElement = listElement->next;
					done = shouldChildCauseWaitToReturn(currentProcess, childProcess, options, status, childProcessId);
				}

				processGroupManagerReleaseReservation(processGroup);
			}

			if (!done) {
				if (options & WNOHANG) {
					done = true;
					*childProcessId = 0;

				} else {
					processManagerChangeProcessState(currentProcess, currentProcess, SUSPENDED_WAITING_CHILD, 0);
					enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
					assert(currentProcess->state == RUNNABLE);

					if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
						result = EINTR;
						done = true;
					} else {
						assert(NORMAL_EXECUTION_RESUMED);
					}
				}
			}

		} else {
			done = true;
			result = ECHILD;
		}

	} while (!done);

	return result;
}

static bool hasCallbackConfiguredForSignal(struct Process* process, int signalId) {
	return process->signalInformation[signalId - 1].callback != NULL;
}

void processServicesNotifyParentAboutChildStateChange(struct Process* currentProcess, struct Process* process,
		enum ProcessState currentState, enum ProcessState newState, int sourceSignalId) {
	if (process->parentProcess != NULL) {
		if (currentState == STOPPED && newState == RUNNABLE) {
			assert(sourceSignalId == SIGCONT || sourceSignalId == SIGKILL);
			process->stateToNotifyParentAbout = RUNNABLE;
			process->signalToNotifyProcessAbout = sourceSignalId;

		} else if (currentState == RUNNABLE && newState == STOPPED) {
			assert(sourceSignalId == SIGSTOP || sourceSignalId == SIGTSTP || sourceSignalId == SIGTTIN || sourceSignalId == SIGTTOU);
			process->stateToNotifyParentAbout = STOPPED;
			process->signalToNotifyProcessAbout = sourceSignalId;
		}

		if (hasCallbackConfiguredForSignal(process->parentProcess, SIGCHLD)) {
			int signalHandlingConfigurationFlags = process->parentProcess->signalInformation[SIGCHLD - 1].signalHandlingConfiguration.sa_flags;
			bool generateSignal = newState == WAITING_EXIT_STATUS_COLLECTION
					|| ((signalHandlingConfigurationFlags & SA_NOCLDSTOP) == 0 && currentState == STOPPED && newState == RUNNABLE)
					|| ((signalHandlingConfigurationFlags & SA_NOCLDSTOP) == 0 && currentState == RUNNABLE && newState == STOPPED);

			if (generateSignal) {
				APIStatusCode result = signalServicesGenerateSignal(currentProcess, process->parentProcess->id, SIGCHLD, false, NULL);
				assert(result == SUCCESS);
			}
		}

		if (process->parentProcess->state == SUSPENDED_WAITING_CHILD
				/* Is it possible that the parent is interested in this change? */
				&& (newState == WAITING_EXIT_STATUS_COLLECTION || (currentState == STOPPED && newState == RUNNABLE) || (currentState == RUNNABLE && newState == STOPPED))) {
			processManagerChangeProcessState(currentProcess, process->parentProcess, RUNNABLE, 0);
		}
	}
}

static void resumeProcessExecutionAfterSleep(struct Process* process) {
	assert(process->sleepCommandSchedulerId != NULL);

	/* Need to check if it is still sleeping. It could have been stopped. */
	if (process->state == SUSPENDED_SLEEPING) {
		processManagerChangeProcessState(processManagerGetCurrentProcess(), process, RUNNABLE, 0);
	}

	process->sleepCommandSchedulerId = NULL;
}

APIStatusCode processServicesSetProcessGroup(struct Process* currentProcess, pid_t processId, pid_t processGroupId) {
	APIStatusCode result = SUCCESS;
	struct Process* targetProcess = NULL;
	if (processId <= 0) {
		targetProcess = currentProcess;
	} else {
		targetProcess = processManagerGetProcessById(processId);
	}

	if (targetProcess != NULL && (targetProcess == currentProcess || targetProcess->parentProcess == currentProcess)) {
		if (!targetProcess->executedExecutableAfterFork || targetProcess == currentProcess) {
			if (processGroupId <= 0) {
				processGroupId = targetProcess->id;
			}

			struct Session* targetProcessSession = processGetSession(targetProcess);
			struct Session* currentProcessSession = processGetSession(currentProcess);

			struct ProcessGroup* processGroup = processGroupManagerGetAndReserveProcessGroupById(processGroupId);

			/* The process indicated by the pid argument is a session leader. */
			if (processIsSessionLeader(targetProcess)

					/*
					 * The value of the pid argument matches the process ID of a child process of the calling process and
					 * the child process is not in the same session as the calling process.
					 */
					|| (targetProcessSession != currentProcessSession)

					/*
					 * The value of the pgid argument is valid but does not match the process ID of the process indicated by
					 * the pid argument and there is no process with a process group ID that matches the value of the pgid argument
					 * in the same session as the calling process.
					 */
					|| (processGroupId != targetProcess->id && (processGroup == NULL || processGroup->session != currentProcessSession))) {
				result = EPERM;

			} else {
				assert(targetProcessSession != NULL);
				assert(currentProcessSession != NULL);
				assert(targetProcessSession == currentProcessSession);

				if (processGroup == NULL) {
					assert(processGroupId == targetProcess->id);
					processGroup = processGroupManagerAcquireProcessGroup(targetProcess);
					if (processGroup == NULL) {
						result = ENOMEM;
					} else {
						sessionManagerInsertProcessGroup(currentProcessSession, processGroup);
					}

				} else if (targetProcess->processGroup != processGroup) {
					processGroupInsertProcess(processGroup, targetProcess);
				}
			}

			if (processGroup != NULL) {
				processGroupManagerReleaseReservation(processGroup);
			}

		} else {
			/*
			 *	An attempt was made to change the process group ID of one of the children of the calling process and the child
			 *	had already performed an execve.
			 *
			 * https://man7.org/linux/man-pages/man2/getpgrp.2.html
			 */
			result = EACCES;
		}

	} else {
		result = ESRCH;
	}

	return result;
}

void processServicesSleep(struct Process* currentProcess, int milliseconds) {
	struct ProcessExecutionState2* processExecutionState2 = currentProcess->processExecutionState2;
	assert(currentProcess->state == RUNNABLE);
	assert(currentProcess->sleepCommandSchedulerId == NULL);

	currentProcess->sleepCommandSchedulerId  = commandSchedulerSchedule(processExecutionState2->ebx, false, (void (*)(void*)) &resumeProcessExecutionAfterSleep, currentProcess);

	bool done = false;

	do {
		processManagerChangeProcessState(currentProcess, currentProcess, SUSPENDED_SLEEPING, 0);
		enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = processManagerScheduleProcessExecution();

		assert(currentProcess->state == RUNNABLE);
		if (currentProcess->sleepCommandSchedulerId == NULL) {
			processExecutionState2->eax = 0;
			done = true;

		} else if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
			processExecutionState2->eax = (uint32_t) (commandSchedulerCancel(currentProcess->sleepCommandSchedulerId) >> 10); /* As we can not divide by 1000. */
			currentProcess->sleepCommandSchedulerId = NULL;
			done = true;
		}

	} while (!done);

	assert(currentProcess->sleepCommandSchedulerId == NULL);
}

void processServicesWakeUpProcesses(struct Process* currentProcess, struct DoubleLinkedList* processList, enum ProcessState processState) {
	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(processList);
	while (listElement != NULL) {
		struct Process* process = processGetProcessFromIOProcessListElement(listElement);
		listElement = listElement->next;

		if (process->state == processState) {
			assert(process->waitingIOProcessList == processList);
			assert(processCountIOEventsBeingMonitored(process) == 0);
			processRemoveFromWaitingIOProcessList(process);
			processManagerChangeProcessState(currentProcess, process, RUNNABLE, 0);
		}
	}
}
