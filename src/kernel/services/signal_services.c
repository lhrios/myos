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

#include <errno.h>
#include <string.h>

#include <sys/wait.h>

#include "kernel/command_scheduler.h"
#include "kernel/log.h"
#include "kernel/process/process_manager.h"
#include "kernel/process/process_group_manager.h"

#include "kernel/services/process_services.h"
#include "kernel/services/signal_services.h"

#include "util/debug_utils.h"
#include "util/iterator.h"

__attribute__ ((cdecl)) enum ResumedProcessExecutionSituation signalServicesHandlePendingSignals(struct Process* currentProcess) {
	enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = NORMAL_EXECUTION_RESUMED;
	while (currentProcess->mightHaveAnySignalToHandle) {
		int signalIdToNotifyAbout;
		switch (signalServicesCalculateSignalToHandle(currentProcess, &signalIdToNotifyAbout)) {
			case CONTINUE_PROCESS_EXECUTION_THEN_USER_CALLBACK:
			case USER_CALLBACK:
				signalServicesNotifyProcessAboutSignal(currentProcess, signalIdToNotifyAbout);
				resumedProcessExecutionSituation = WILL_CALL_SIGNAL_HANDLER;
				break;

			case TERMINATE_PROCESS:
				resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
				assert(false);
				break;

			case DO_NOTHING:
			case CONTINUE_PROCESS_EXECUTION:
				break;

			case STOP_PROCESS:
				resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
			break;

			default:
				assert(false);
				break;
		}
	}

	return resumedProcessExecutionSituation;
}

APIStatusCode signalServicesChangeSignalAction(struct Process* process, void (*callback)(int, struct sigaction*), int signalId, struct sigaction* newSignalHandlingConfiguration,
	struct sigaction* oldSignalHandlingConfiguration) {

	APIStatusCode result = SUCCESS;

	if ((1 <= signalId && signalId <= NUMBER_OF_SIGNALS) && signalId != SIGKILL && signalId != SIGSTOP) {
		struct SignalInformation* signalInformation = &process->signalInformation[signalId - 1];

		if (newSignalHandlingConfiguration != NULL) {
			if (oldSignalHandlingConfiguration != NULL) {
				if (signalInformation->callback != NULL) {
					memcpy(oldSignalHandlingConfiguration, &signalInformation->signalHandlingConfiguration, sizeof(struct sigaction));
				} else {
					memset(oldSignalHandlingConfiguration, 0, sizeof(struct sigaction));
					if (signalInformation->ignored) {
						oldSignalHandlingConfiguration->sa_handler = SIG_IGN;
					} else {
						oldSignalHandlingConfiguration->sa_handler = SIG_DFL;
					}
				}
			}

			if (newSignalHandlingConfiguration->sa_handler == SIG_DFL) {
				signalInformation->callback = NULL;
				signalInformation->ignored = false;

			} else if (newSignalHandlingConfiguration->sa_handler == SIG_IGN) {
				signalInformation->callback = NULL;
				signalInformation->ignored = true;

			} else {
				memcpy(&signalInformation->signalHandlingConfiguration, newSignalHandlingConfiguration, sizeof(struct sigaction));
				signalInformation->signalHandlingConfiguration.sa_sigaction = NULL; /* As it is not supported yet. */
				signalInformation->callback = callback;
				signalInformation->ignored = false;
			}

		} else if (oldSignalHandlingConfiguration != NULL) {
			memset(oldSignalHandlingConfiguration, 0, sizeof(struct sigaction));

			if (signalInformation->callback != NULL) {
				memcpy(oldSignalHandlingConfiguration, &signalInformation->signalHandlingConfiguration, sizeof(struct sigaction));

			} else if (signalInformation->ignored) {
				oldSignalHandlingConfiguration->sa_handler = SIG_IGN;

			} else {
				oldSignalHandlingConfiguration->sa_handler = SIG_DFL;
			}
		}

	} else {
		result = EINVAL;
	}

	return result;
}

#define REQUIRED_MEMORY_TO_SIGNAL_CALLBACK (sizeof(void*) + sizeof(struct sigaction) + sizeof(uint32_t) + sizeof(sigset_t) + sizeof(uint32_t)) /* bytes */
static void prepareProcessStackToSignalCallback(struct Process* process, int signalIdToNotifyAbout, struct SignalInformation* signalInformation) {
	struct ProcessExecutionState1* processExecutionState1 = process->processExecutionState1;

	assert(processExecutionState1->esp3 % sizeof(uint32_t) == 0);
	uint32_t esp3 = processExecutionState1->esp3 / sizeof(uint32_t);
	uint32_t* stack = (void*) NULL;

	stack[--esp3] = processExecutionState1->eip;

	esp3 -= sizeof(struct sigaction) / sizeof(uint32_t);
	struct sigaction* signalHandlingConfiguration = (void*) &stack[esp3];
	memcpy(signalHandlingConfiguration, &signalInformation->signalHandlingConfiguration, sizeof(struct sigaction));

	stack[--esp3] = sizeof(struct sigaction);

	esp3 -= sizeof(sigset_t) / sizeof(uint32_t);
	sigset_t* blockedSignalsSet = (void*) &stack[esp3];
	memcpy(blockedSignalsSet, &signalHandlingConfiguration->sa_mask, sizeof(sigset_t));

	/* Signals that are already blocked will not be blocked again. */
	sigdelall(blockedSignalsSet, &process->blockedSignalsSet);
	if ((signalHandlingConfiguration->sa_flags & SA_NODEFER) == 0 && (*blockedSignalsSet & signalIdToNotifyAbout) == 0) {
		sigaddset(blockedSignalsSet, signalIdToNotifyAbout);
	}

	process->blockedSignalsSet = *blockedSignalsSet | process->blockedSignalsSet;

	stack[--esp3] = (uint32_t) signalIdToNotifyAbout;

	assert(esp3 * sizeof(uint32_t) == processExecutionState1->esp3 - REQUIRED_MEMORY_TO_SIGNAL_CALLBACK);
	processExecutionState1->esp3 = esp3 * sizeof(uint32_t);
	processExecutionState1->eip = (uint32_t) signalInformation->callback;
}

static void forceProcessTerminationDueToSignal(struct Process* currentProcess, int signalId) {
	int exitStatus = WIFSIGNALED_MASK | (WTERMSIG_MASK & (signalId << WTERMSIG_SHIFT));
	processManagerTerminate(currentProcess, exitStatus, signalId);
}

void signalServicesNotifyProcessAboutSignal(struct Process* process, int signalIdToNotifyAbout) {
	struct SignalInformation* signalInformation = &process->signalInformation[signalIdToNotifyAbout - 1];

	assert(signalInformation->callback != NULL);
	prepareProcessStackToSignalCallback(process, signalIdToNotifyAbout, signalInformation);
}

static enum SignalAction getSignalAction(struct Process* process, int signalId) {
	struct SignalInformation* signalInformation = &process->signalInformation[signalId - 1];

	if (signalInformation->signalCreationInformation.inResponseToUnrecoverableFault) {
		return TERMINATE_PROCESS;

	} else {
		switch (signalId) {
			case SIGWINCH:
			case SIGURG:
			case SIGCHLD:
				return signalInformation->callback != NULL ?  USER_CALLBACK : DO_NOTHING;

			case SIGUSR1:
			case SIGUSR2:
			case SIGFPE:
			case SIGVTALRM:
			case SIGALRM:
			case SIGTRAP:
			case SIGSYS:
			case SIGXCPU:
			case SIGXFSZ:
			case SIGPIPE:
			case SIGABRT:
			case SIGBUS:
			case SIGILL:
			case SIGINT:
			case SIGSEGV:
			case SIGTERM:
			case SIGQUIT:
			case SIGPROF:
			case SIGHUP:
				return signalInformation->callback != NULL ? USER_CALLBACK : TERMINATE_PROCESS;

			case SIGTTOU:
			case SIGTTIN:
			case SIGTSTP:
				return signalInformation->callback != NULL ? USER_CALLBACK : STOP_PROCESS;

			case SIGSTOP:
				assert(signalInformation->callback == NULL);
				return STOP_PROCESS;

			case SIGKILL:
				assert(signalInformation->callback == NULL);
				return TERMINATE_PROCESS;

			case SIGCONT:
				return signalInformation->callback != NULL ? CONTINUE_PROCESS_EXECUTION_THEN_USER_CALLBACK : CONTINUE_PROCESS_EXECUTION;

			default:
				assert(false);
				return TERMINATE_PROCESS;
		}
	}
}

static bool canDeliverSignal(struct Process* process, int signalId) {
	bool result = false;
	struct SignalInformation* signalInformation = &process->signalInformation[signalId - 1];
	if (signalInformation->pending) {
		if (signalId == SIGFPE || signalId == SIGILL || signalId == SIGSEGV || signalId == SIGSTOP || signalId == SIGCONT || signalId == SIGKILL) {
			/* As it is not allowed to block those signals. */
			result = true;

		} else {
			if (signalInformation->signalCreationInformation.inResponseToUnrecoverableFault) {
				result = true;

			} else {
				if (!sigismember(&process->blockedSignalsSet, signalId)) {
					if (signalInformation->ignored) {
						signalInformation->pending = false;

					} else {
						result = true;
					}
				}
			}
		}
	}

	return result;
}

enum SignalAction signalServicesCalculateSignalToHandle(struct Process* currentProcess, int* outputSignalIdToNotifyAbout) {
	assert(currentProcess->mightHaveAnySignalToHandle);

	currentProcess->mightHaveAnySignalToHandle = false;
	int signalIdToNotifyAbout = -1;

	/* First, select a signal generated by the kernel in response to an unrecoverable fault. */
	for (int signalId = NUMBER_OF_SIGNALS; signalId > 0 && signalIdToNotifyAbout == -1; signalId--) {
		struct SignalInformation* signalInformation = &currentProcess->signalInformation[signalId - 1];
		if ((signalId == SIGKILL || signalInformation->signalCreationInformation.inResponseToUnrecoverableFault)
				&& canDeliverSignal(currentProcess, signalId)) {
			signalInformation->pending = false;
			signalIdToNotifyAbout = signalId;
		}
	}

	/* Finally, select a any signal. Also make sure to update if there is another signal to be delivered. */
	for (int signalId = NUMBER_OF_SIGNALS; signalId > 0 && (signalIdToNotifyAbout == -1 || !currentProcess->mightHaveAnySignalToHandle); signalId--) {
		struct SignalInformation* signalInformation = &currentProcess->signalInformation[signalId - 1];
		if (canDeliverSignal(currentProcess, signalId)) {
			if (signalIdToNotifyAbout == -1) {
				signalInformation->pending = false;
				signalIdToNotifyAbout = signalId;
			} else {
				currentProcess->mightHaveAnySignalToHandle = true;
			}
		}
	}

	enum SignalAction signalAction = DO_NOTHING;
	if (signalIdToNotifyAbout != -1) {
		 if (!processIsValidSegmentAccess(currentProcess, currentProcess->processExecutionState1->esp3 - REQUIRED_MEMORY_TO_SIGNAL_CALLBACK, REQUIRED_MEMORY_TO_SIGNAL_CALLBACK)) {
			 signalAction = TERMINATE_PROCESS;
		 }	else {
			 signalAction = getSignalAction(currentProcess, signalIdToNotifyAbout);
		 }

		 switch(signalAction) {
			 case TERMINATE_PROCESS:
				forceProcessTerminationDueToSignal(currentProcess, signalIdToNotifyAbout);
				signalIdToNotifyAbout = 0;
				break;

			 case DO_NOTHING:
				 signalIdToNotifyAbout = 0;
				 break;

			 case STOP_PROCESS:
				 processManagerStop(currentProcess, signalIdToNotifyAbout);
				 signalIdToNotifyAbout = 0;
				 break;

			 case CONTINUE_PROCESS_EXECUTION_THEN_USER_CALLBACK:
				 processManagerChangeProcessState(currentProcess, currentProcess, RUNNABLE, signalIdToNotifyAbout);
				 break;

			 case CONTINUE_PROCESS_EXECUTION:
				 processManagerChangeProcessState(currentProcess, currentProcess, RUNNABLE, signalIdToNotifyAbout);
				 signalIdToNotifyAbout = 0;
				 break;

			 case USER_CALLBACK:
				 break;

			 default:
				 assert(false);
				 break;
		 }
	}

	*outputSignalIdToNotifyAbout = signalIdToNotifyAbout;
	return signalAction;
}

bool signalServicesIsSignalIgnored(struct Process* process, int signalId) {
	return process->signalInformation[signalId - 1].ignored;
}

bool signalServicesIsSignalBlocked(struct Process* process, int signalId) {
	return sigismember(&process->blockedSignalsSet, signalId);
}

static void discardSignal(struct Process* process, int signalId) {
	struct SignalInformation* signalInformation = &process->signalInformation[signalId - 1];
	signalInformation->pending = false;
}

static APIStatusCode doGenerateSignal(struct Process* senderProcess, struct Process* receiverProcess, int signalId, bool inResponseToUnrecoverableFault,
		enum ResumedProcessExecutionSituation* resumedProcessExecutionSituation) {
	APIStatusCode result = SUCCESS;

	if (receiverProcess == NULL || receiverProcess->state == WAITING_EXIT_STATUS_COLLECTION) {
		result = ESRCH;

	} else if (signalId == 0) {
		result = SUCCESS;

	} else if (1 <= signalId && signalId <= NUMBER_OF_SIGNALS) {
		struct SignalInformation* signalInformation = &receiverProcess->signalInformation[signalId - 1];

		if (!signalInformation->pending || inResponseToUnrecoverableFault) {
			/* The "init" is protected. Therefore, it just receive signals if it there is a signal handler. */
			if (receiverProcess->id != 1 || (receiverProcess->id == 1 && signalInformation->callback != NULL)) {
				memset(&signalInformation->signalCreationInformation, 0, sizeof(siginfo_t));
				signalInformation->signalCreationInformation.inResponseToUnrecoverableFault = inResponseToUnrecoverableFault;

				signalInformation->pending = true;
				receiverProcess->mightHaveAnySignalToHandle = true;

				/*
				 * When any stop signal (SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU) is generated for a process or thread, all pending SIGCONT signals for that process or any of the threads within that process shall be discarded.
				 * Conversely, when SIGCONT is generated for a process or thread, all pending stop signals for that process or any of the threads within that process shall be discarded.
				 *
				 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_04
				 */
				if (signalId == SIGCONT) {
					discardSignal(receiverProcess, SIGSTOP);
					discardSignal(receiverProcess, SIGTSTP);
					discardSignal(receiverProcess, SIGTTIN);
					discardSignal(receiverProcess, SIGTTOU);

				} else if (signalId == SIGSTOP || signalId == SIGTSTP || signalId == SIGTTIN || signalId == SIGTTOU) {
					discardSignal(receiverProcess, SIGCONT);
				}

				/* Is it a "raise"? It can not return before handling the signal. */
				if (senderProcess == receiverProcess) {
					assert(receiverProcess->state == RUNNABLE);
					enum ResumedProcessExecutionSituation localResumedProcessExecutionSituation;
					localResumedProcessExecutionSituation = signalServicesHandlePendingSignals(receiverProcess);
					if (resumedProcessExecutionSituation != NULL) {
						*resumedProcessExecutionSituation = localResumedProcessExecutionSituation;
					}

				} else {
					/*
					 * While a process is stopped, no more signals can be delivered to it until it is continued, except SIGKILL signals and (obviously) SIGCONT signals.
					 * The signals are marked as pending, but not delivered until the process is continued.
					 *
					 * https://www.gnu.org/software/libc/manual/html_node/Job-Control-Signals.html
					 */
					if (receiverProcess->state != STOPPED || (signalId == SIGCONT || signalId == SIGKILL)) {
						processManagerChangeProcessState(senderProcess, receiverProcess, RUNNABLE, signalId);
					}
				}

			} else {
				result = EINVAL;
			}
		}

	} else {
		result = EINVAL;
	}

	return result;
}

static void* transformValueBeforeNextReturn(struct Iterator* iterator, void* value) {
	return processGetProcessFromProcessGroupListElement(value);
}

APIStatusCode signalServicesGenerateSignal(struct Process* senderProcess, pid_t scope, int signalId, bool inResponseToUnrecoverableFault,
		enum ResumedProcessExecutionSituation* resumedProcessExecutionSituation) {
	APIStatusCode result = SUCCESS;

	if (0 <= signalId && signalId <= NUMBER_OF_SIGNALS) {
		bool checkExistenceAndPermission = signalId == 0;

		if (scope <= 0) {
			struct Iterator* iterator = NULL;
			struct FixedCapacitySortedArrayIterator fixedCapacitySortedArrayIterator;
			struct DoubleLinkedListIterator doubleLinkedListIterator;

			struct ProcessGroup* processGroup = NULL;

			if (scope == 0 || scope < -1) {
				pid_t processGroupId;
				if (scope == 0) {
					processGroupId = senderProcess->processGroup->id;
				} else {
					processGroupId = -scope;
				}

				processGroup = processGroupManagerGetAndReserveProcessGroupById(processGroupId);
				if (processGroup != NULL) {
					doubleLinkedListInitializeIterator(&processGroup->processesList, &doubleLinkedListIterator);
					doubleLinkedListIterator.iterator.transformValueBeforeNextReturn = &transformValueBeforeNextReturn;
					iterator = &doubleLinkedListIterator.iterator;
				}

			} else {
				processManagerInitializeAllProcessesIterator(&fixedCapacitySortedArrayIterator);
				iterator = &fixedCapacitySortedArrayIterator.iterator;
			}

			if (iterator != NULL) {
				bool atLeastOneSignalSent = false;
				int noPermissionToSendSignalCount = 0;
				int processesCount = 0;

				while (iteratorHasNext(iterator)) {
					processesCount++;

					struct Process* receiverProcess = iteratorNext(iterator);
					result = doGenerateSignal(senderProcess, receiverProcess, signalId, inResponseToUnrecoverableFault, resumedProcessExecutionSituation);
					if (result == SUCCESS) {
						atLeastOneSignalSent = true;
						if (checkExistenceAndPermission) {
							break;
						}

					} else if (result == EPERM) {
						noPermissionToSendSignalCount++;
					}
				}

				if (processesCount > 0) {
					if (atLeastOneSignalSent) {
						result = SUCCESS;

					} else if (noPermissionToSendSignalCount == processesCount) {
						result = EPERM;

					} else {
						result = ESRCH;
					}

				} else {
					result = ESRCH;
				}

			} else {
				result = ESRCH;
			}

			if (processGroup != NULL) {
				processGroupManagerReleaseReservation(processGroup);
			}

		} else {
			struct Process* receiverProcess = processManagerGetProcessById(scope);
			result = doGenerateSignal(senderProcess, receiverProcess, signalId, inResponseToUnrecoverableFault, resumedProcessExecutionSituation);
		}

	} else {
		result = EINVAL;
	}

	return result;
}

APIStatusCode signalServicesChangeSignalsBlockage(struct Process* process, int how, const sigset_t* newSignalSet, sigset_t* oldBlockedSignalsSet) {
	APIStatusCode result = SUCCESS;

	if (how == SIG_BLOCK || how == SIG_UNBLOCK || how == SIG_SETMASK) {
		sigset_t internalOldBlockedSignalsSet = process->blockedSignalsSet;
		if (newSignalSet != NULL) {
			switch (how) {
				case SIG_BLOCK:
					{
						sigset_t newBlockedSignalsSet;
						newBlockedSignalsSet = *newSignalSet;
						process->blockedSignalsSet = sigcombine(&process->blockedSignalsSet, &newBlockedSignalsSet);

					}
					break;

				case SIG_UNBLOCK: {
						process->blockedSignalsSet = sigdelall(&process->blockedSignalsSet, newSignalSet);
					}
					break;

				case SIG_SETMASK:
					{
						sigset_t newBlockedSignalsSet;
						newBlockedSignalsSet = *newSignalSet;
						process->blockedSignalsSet = newBlockedSignalsSet;
					}
					break;

				default:
					assert(false);
					break;
			}
		}

		if (oldBlockedSignalsSet != NULL) {
			memcpy(oldBlockedSignalsSet, &internalOldBlockedSignalsSet, sizeof(sigset_t));
		}

	} else {
		result = EINVAL;
	}

	return result;
}
