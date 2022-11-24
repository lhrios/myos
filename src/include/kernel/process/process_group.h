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

#ifndef KERNEL_PROCESS_GROUP_H
	#define KERNEL_PROCESS_GROUP_H

	#include <stdbool.h>
	#include <stdint.h>

	#include "kernel/process/process_group_manager.h"

	#include "kernel/io/virtual_file_system_node.h"

	#include "util/double_linked_list.h"
	#include "util/fixed_capacity_sorted_array.h"

	struct Process;
	enum ProcessState;
	struct Session;
	struct ProcessGroup {
		struct DoubleLinkedListElement doubleLinkedListElement;
		pid_t id;
		struct Process* leaderProcess;
		struct Session* session;
		struct DoubleLinkedList processesList;
		uint32_t usageCount;
	};

	void processGroupInsertProcess(struct ProcessGroup* processGroup, struct Process* process);
	void processGroupRemoveProcess(struct Process* process);
	bool processGroupIsOrphaned(struct ProcessGroup* processGroup);
	int processGroupCountWithState(struct ProcessGroup* processGroup, enum ProcessState processState);

#endif
