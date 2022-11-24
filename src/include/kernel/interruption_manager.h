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

#ifndef KERNEL_INTERRUPTION_MANAGER_H
	#define KERNEL_INTERRUPTION_MANAGER_H

	#include <stdbool.h>
	#include <stdint.h>

	#include "kernel/priority.h"
	#include "kernel/process/process_execution_state.h"

	void interruptionManagerInitialize(uint32_t interruptionVectorToHandleSystemCall);
	void interruptionManagerRegisterInterruptionHandler(uint8_t interruptionVector,
		void (*handler)(uint32_t, struct ProcessExecutionState1*, struct ProcessExecutionState2*));
	void interruptionManagerRegisterSystemCallHandler(void (*systemCallHandler)(struct ProcessExecutionState1*, struct ProcessExecutionState2*));

	void __attribute__ ((cdecl)) interruptionManagerHandler(uint32_t, struct ProcessExecutionState1*, struct ProcessExecutionState2*);
	void __attribute__ ((cdecl)) interruptionManagerSystemCallHandler(struct ProcessExecutionState1*, struct ProcessExecutionState2*);
	bool interruptionManagerRegisterCommandToRunAfterInterruptionHandler(int priority, void (*command)(void*), void* argument);
	void __attribute__ ((cdecl)) interruptionManagerRunCommandsAfterInterruptionHandler(void);
#endif