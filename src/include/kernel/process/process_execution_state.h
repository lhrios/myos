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

#ifndef KERNEL_PROCESS_EXECUTION_STATE_H
	#define KERNEL_PROCESS_EXECUTION_STATE_H

	#include <stdint.h>

	struct ProcessExecutionState2 {
		uint32_t ds;
		uint32_t edi;
		uint32_t esi;
		uint32_t ebp;
		uint32_t esp0;
		uint32_t ebx;
		uint32_t edx;
		uint32_t ecx;
		uint32_t eax;
		uint32_t interruptionVector;
	} __attribute__((packed));

	struct ProcessExecutionState1 {
		uint32_t eip;
		uint32_t cs;
		uint32_t eflags;
		uint32_t esp3;
		uint32_t ss;
	} __attribute__((packed));

#endif
