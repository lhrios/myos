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

#ifndef KERNEL_SIGNAL_SERVICES_H
	#define KERNEL_SIGNAL_SERVICES_H

	#include <stdbool.h>
	#include <signal.h>

	#include <sys/types.h>

	#include "kernel/api_status_code.h"
	#include "kernel/process/process.h"

	enum SignalAction {
		TERMINATE_PROCESS,
		DO_NOTHING,
		STOP_PROCESS,
		USER_CALLBACK,
		CONTINUE_PROCESS_EXECUTION,
		CONTINUE_PROCESS_EXECUTION_THEN_USER_CALLBACK
	};

	APIStatusCode signalServicesChangeSignalAction(struct Process* process, void (*callback)(int, struct sigaction*), int signalId, struct sigaction* newSignalHandlingConfiguration,
		struct sigaction* oldSignalHandlingConfiguration);
	APIStatusCode signalServicesGenerateSignal(struct Process* senderProcess, pid_t receiverProcessId, int signalId, bool inResponseToUnrecoverableFault,
		enum ResumedProcessExecutionSituation* resumedProcessExecutionSituatio);
	APIStatusCode signalServicesChangeSignalsBlockage(struct Process* process, int how, const sigset_t* newSignalSet, sigset_t* oldBlockedSignalsSet);
	__attribute__ ((cdecl)) enum ResumedProcessExecutionSituation signalServicesHandlePendingSignals(struct Process* process);
	enum SignalAction signalServicesCalculateSignalToHandle(struct Process* currentProcess, int* signalIdToNotifyAbout);
	void signalServicesNotifyProcessAboutSignal(struct Process* process, int signalIdToNotifyAbout);
	bool signalServicesCanDeliverSignal(struct Process* process, int signalId);
	bool signalServicesIsSignalIgnored(struct Process* process, int signalId);
	bool signalServicesIsSignalBlocked(struct Process* process, int signalId);

#endif
