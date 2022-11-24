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

#include <stdint.h>
#include <stdlib.h>

#include "kernel/command_scheduler.h"
#include "kernel/pit.h"

#include "util/priority_queue.h"

struct ScheduledCommand {
	uint64_t upTimeInMilliseconds;
	uint64_t delayInMilliseconds;
	bool repeat;
	void (*command)(void*);
	void* argument;
	bool canceled;
};

#define REGISTERED_SCHEDULED_COMMANDS_ARRAY_LENGTH 512
static struct ScheduledCommand registeredScheduledCommands[REGISTERED_SCHEDULED_COMMANDS_ARRAY_LENGTH];
static struct ScheduledCommand* queue[REGISTERED_SCHEDULED_COMMANDS_ARRAY_LENGTH];
static struct PriorityQueue priorityQueue;

static bool scheduledCommandComparator(const void* scheduledCommand1, const void* scheduledCommand2, void* argument1, void* argument2, void* argument3) {
	return (*((struct ScheduledCommand**) scheduledCommand1))->upTimeInMilliseconds < (*((struct ScheduledCommand**) scheduledCommand2))->upTimeInMilliseconds;
}

static void commandSchedulerTick(uint64_t tickCount, uint64_t upTimeInMilliseconds) {
	while (priorityQueueSize(&priorityQueue) > 0) {
		struct ScheduledCommand* scheduledCommand;
		priorityQueuePeek(&priorityQueue, &scheduledCommand);
		if (scheduledCommand->upTimeInMilliseconds <= upTimeInMilliseconds) {
			 priorityQueueRemove(&priorityQueue, &scheduledCommand);
			if (!scheduledCommand->canceled) {
				scheduledCommand->command(scheduledCommand->argument);

				if (scheduledCommand->repeat) {
					scheduledCommand->upTimeInMilliseconds = scheduledCommand->delayInMilliseconds + upTimeInMilliseconds;
					bool result = priorityQueueInsert(&priorityQueue, &scheduledCommand);
					assert(result);
				}
			}

		} else {
			break;
		}
	}
}

uint64_t commandSchedulerCancel(void* commandId) {
	struct ScheduledCommand* scheduledCommand = commandId;
	scheduledCommand->canceled = true;
	uint64_t upTimeInMilliseconds = pitGetUpTimeInMilliseconds();
	if (scheduledCommand->upTimeInMilliseconds >= upTimeInMilliseconds) {
		return scheduledCommand->upTimeInMilliseconds - upTimeInMilliseconds;
	} else {
		return 0;
	}
}

void commandSchedulerInitialize(void) {
	for (int i = 0; i < REGISTERED_SCHEDULED_COMMANDS_ARRAY_LENGTH; i++) {
		queue[i] = &registeredScheduledCommands[i];
	}
	priorityQueueInitialize(&priorityQueue, queue, REGISTERED_SCHEDULED_COMMANDS_ARRAY_LENGTH,
		0, sizeof(struct ScheduledCommand*), &scheduledCommandComparator, NULL, NULL, NULL);

	pitRegisterCommandToRunOnTick(&commandSchedulerTick);
}

void* commandSchedulerSchedule(uint64_t delayInMilliseconds, bool repeat, void (*command)(void*), void* argument) {
	int size = priorityQueueSize(&priorityQueue);

	if (size + 1 < REGISTERED_SCHEDULED_COMMANDS_ARRAY_LENGTH) {
		struct ScheduledCommand* scheduledCommand = queue[size];
		scheduledCommand->upTimeInMilliseconds = pitGetUpTimeInMilliseconds() + delayInMilliseconds;
		scheduledCommand->delayInMilliseconds = delayInMilliseconds;
		scheduledCommand->repeat = repeat;
		scheduledCommand->command = command;
		scheduledCommand->argument = argument;
		scheduledCommand->canceled = false;

		bool result = priorityQueueInsert(&priorityQueue, &scheduledCommand);
		assert(result);
		return scheduledCommand;

	} else {
		return NULL;
	}
}
