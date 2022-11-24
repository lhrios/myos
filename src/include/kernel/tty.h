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

#ifndef KERNEL_TTY_H
	#define KERNEL_TTY_H

	#include <stdbool.h>

	#include "kernel/multiboot.h"
	#include "kernel/process/process.h"
	#include "kernel/process/process_group.h"
	#include "kernel/vga_colors.h"

	#define TTY_COUNT 4

	void ttyInitialize(struct multiboot_info *multiboot_info);
	void ttyRegisterDevices(void);

	void ttyWriteToOutputVaFormat(int ttyId, const char* format, va_list ap);
	void ttyWriteToOutputFormat(int ttyId, const char* format, ...);
	void ttyHandleSessionLeaderTermination(int ttyId, struct Process* currentProcess);
	void ttyHandleProcessGroupBecameEmpty(struct ProcessGroup* processGroup);
	bool ttyIsValidTTYId(int ttyId);
	bool ttySetForegroundTTY(int ttyId);
	APIStatusCode ttyPrintDebugReport(void);
#endif
