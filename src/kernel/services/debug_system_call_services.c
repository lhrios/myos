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

#include <string.h>

#include "kernel/memory_manager.h"
#include "kernel/process/process_group_manager.h"
#include "kernel/process/process_manager.h"
#include "kernel/session_manager.h"
#include "kernel/tty.h"

#include "kernel/io/block_cache_manager.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "kernel/services/debug_system_call_services.h"
#include "kernel/services/process_services.h"

APIStatusCode debugSystemCallServicesLogKernelModuleDebugReport(struct Process* process, const char* kernelModuleName) {
	APIStatusCode result = SUCCESS;

	if (processIsValidSegmentAccess(process, (uint32_t) kernelModuleName, sizeof(char))) {
		if (strcmp("block_cache_manager", kernelModuleName) == 0) {
			result = blockCacheManagerPrintDebugReport();
		} else if (strcmp("process_manager", kernelModuleName) == 0) {
			result = processManagerPrintDebugReport();
		} else if (strcmp("session_manager", kernelModuleName) == 0) {
			result = sessionManagerPrintDebugReport();
		} else if (strcmp("memory_manager", kernelModuleName) == 0) {
			result = memoryManagerPrintDebugReport();
		} else if (strcmp("process_group_manager", kernelModuleName) == 0) {
			result = processGroupManagerPrintDebugReport();
		} else if (strcmp("virtual_file_system_manager", kernelModuleName) == 0) {
			result = virtualFileSystemManagerPrintDebugReport();
		} else if (strcmp("tty", kernelModuleName) == 0) {
			result = ttyPrintDebugReport();
		}

	} else {
		result = EFAULT;
	}

	return result;
}
