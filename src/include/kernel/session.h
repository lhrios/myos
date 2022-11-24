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

#ifndef KERNEL_SESSION_H
	#define KERNEL_SESSION_H

	#include <stdint.h>

	#include "util/double_linked_list.h"

	#include "kernel/io/virtual_file_system_node.h"

	struct Process;
	struct Session {
		struct DoubleLinkedListElement doubleLinkedListElement;
		pid_t id;
		struct Process* leaderProcess;
		int controllingTTYId;
		uint32_t usageCount;
	};

#endif
