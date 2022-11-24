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

#include <errno.h>

#include <myos.h>

#include "kernel/system_calls.h"

#define STR(x) #x
#define XSTR(x) STR(x)

void myosInvalidSystemCall(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_INEXISTENT)
		: "memory");
}

void myosSystemAssert(bool value) {
	if (!value) {
		__asm__ __volatile__(
			"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
			:
			: "a"(SYSTEM_CALL_ASSERT_FALSE)
			: "memory");
	}
}

void myosSystemBusyWait(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_BUSY_WAIT)
		: "memory");
}

void myosForceKernelSIGSEGV(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_FORCE_KERNEL_SIGSEGV)
		: "memory");
}

void myosForceKernelSIGILL(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_FORCE_KERNEL_SIGILL)
		: "memory");
}

int myosLogKernelModuleDebugReport(const char* moduleName) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_LOG_KERNEL_MODULE_DEBUG_REPORT), "b"(moduleName)
		: "memory");

	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

void myosFlushAndClearCache(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_CACHE_FLUSH_AND_CLEAR)
		: "memory");
}

void myosFlushCache(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_CACHE_FLUSH)
		: "memory");
}

void myosReboot(void) {
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		:
		: "a"(SYSTEM_CALL_REBOOT)
		: "memory");
}

int myosGetProcessMemorySegmentsLimits(struct ProcessMemorySegmentsLimits* processMemorySegmentsLimits) {
	int result = 0;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result)
		: "a"(SYSTEM_CALL_GET_PROCESS_MEMORY_SEGMENTS_LIMITS), "b"(processMemorySegmentsLimits)
		: "memory");
	if (result) {
		errno = result;
		return -1;
	} else {
		return 0;
	}
}

pid_t myosForkAndGenerateSignal(int signalId) {
	int result;
	pid_t processId;
	__asm__ __volatile__(
		"int $" XSTR(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL) ";"
		: "=a"(result), "=b"(processId)
		: "a"(SYSTEM_CALL_FORK), "b"(signalId)
		: "memory");
	if (result) {
		errno = result;
		return (pid_t)-1;
	} else {
		return processId;
	}
}
