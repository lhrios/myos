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

#include "kernel/busy_waiting_manager.h"
#include "kernel/kernel_life_cycle.h"
#include "kernel/keyboard.h"
#include "kernel/log.h"
#include "kernel/pit.h"

#include "kernel/io/block_cache_manager.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "kernel/tty.h"

void kernelLifeCycleReboot(void) {
	logBringLogTTYToForeground();

	/* This function can never return as the kernel will not be in a valid state after it starts. */
	logInfo("The reboot command has been issued!");

	pitStopTickGenerator();

	virtualFileSystemManagerCloseAllOpenFileDescriptions();
	virtualFileSystemManagerUnmountAllFileSystems();
	blockCacheManageFlush();

	//busyWaitingSleep(10000);

	keyboardReboot();
}
