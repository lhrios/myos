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

#ifndef MYOS_H

	#define MYOS_H

	#define MYOS

	#define IN_MEMORY_DEVICE_ID 1
	#define ATA_DEVICE_ID 2
	#define ATA_DEVICE_PARTITION_ID 3
	#define TTY_DEVICE_ID 4
	#define ZERO_DEVICE_ID 5
	#define NULL_DEVICE_ID 6
	#define PIPE_ID 7
	#define DEVICES_FILE_SYSTEM_ID 8

	#include <assert.h>
	#include <limits.h>
	#include <stdint.h>
	#include <stdbool.h>

	#include <sys/types.h>

	struct ProcessMemorySegmentLimits {
		bool isEmpty;
		uint32_t firstAddress;
		uint32_t lastAddress;
	};

	struct ProcessMemorySegmentsLimits {
		struct ProcessMemorySegmentLimits code;
		struct ProcessMemorySegmentLimits data;
		struct ProcessMemorySegmentLimits stack;
	};

	#ifndef KERNEL_CODE
		int myosGetProcessMemorySegmentsLimits(struct ProcessMemorySegmentsLimits* processMemorySegmentsLimits);
		void myosSystemAssert(bool value);
		void myosSystemBusyWait(void);
		void myosInvalidSystemCall(void);
		void myosForceKernelSIGSEGV(void);
		void myosForceKernelSIGILL(void);
		int myosLogKernelModuleDebugReport(const char* moduleName);
		void myosFlushAndClearCache(void);
		void myosFlushCache(void);
		void myosReboot(void);
		pid_t myosForkAndGenerateSignal(int signalId);
	#endif

	inline __attribute__((always_inline)) dev_t myosCalculateUniqueId(dev_t st_dev, ino_t st_ino) {
		assert(st_dev <= UCHAR_MAX);
		assert((st_ino & 0xFF000000) == 0);

		return (st_dev << 24) | st_ino;
	}

	inline __attribute__((always_inline)) uint64_t x86GetTimeStampCount(void) {
		uint32_t resultLower;
		uint32_t resultUpper;
		__asm__ __volatile__(
			"rdtsc"
			: "=a"(resultLower), "=d"(resultUpper)
			:
			:);
		return (((uint64_t) resultUpper) << 32) | resultLower;
	}
#endif
