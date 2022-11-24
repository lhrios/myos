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

#include <stdbool.h>
#include <string.h>

#include "kernel/error_handler.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"

#include "kernel/process/init_process_creator.h"
#include "kernel/process/process_manager.h"

#include "kernel/services/process_services.h"

#include "util/string_stream_writer.h"

#define ARGUMENTS_BUFFER_SIZE 2048
#define ARGUMENTS_LENGTH 128

#define INIT_EXECUTABLE_PATH "/sbin/init"

struct InitInitializationArgument {
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];
	const char* arguments[ARGUMENTS_LENGTH];
	int argumentCount;
};
_Static_assert(sizeof(struct InitInitializationArgument) <= PAGE_FRAME_SIZE,
	"Expecting that a InitInitializationArgument instance fits on a page frame block.");

static void __attribute__ ((cdecl)) initInitializationCallback(struct InitInitializationArgument* initInitializationArgument) {
	logDebug("Running initInitializationCallback");

	struct Process* currentProcess = processManagerGetCurrentProcess();

	APIStatusCode result = processServicesExecuteExecutable(currentProcess, false, INIT_EXECUTABLE_PATH,
		initInitializationArgument->arguments, NULL);
	if (result != SUCCESS) {
		errorHandlerFatalError("Failed to run \"exec\" for init process: %s", sys_errlist[result]);
	}

	memoryManagerReleasePageFrame(memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) initInitializationArgument), -1);
}

APIStatusCode initProcessCreatorCreate(int initArgc, const char** initArgv) {
	APIStatusCode result = SUCCESS;

	/*
	 * Create and populate the InitInitializationArgument.
	 * We need to allocate a dedicated object as it will be passed to a different process.
	 */
	struct InitInitializationArgument* initInitializationArgument;
	struct DoubleLinkedListElement* doubleLinkedListElement;

	struct StringStreamWriter stringStreamWriter;

	doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
	if (doubleLinkedListElement != NULL) {
		initInitializationArgument = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
	} else {
		result = ENOMEM;
	}

	if (result == SUCCESS) {
		int argumentCount = 0;
		stringStreamWriterInitialize(&stringStreamWriter, initInitializationArgument->argumentsBuffer, ARGUMENTS_BUFFER_SIZE);

		initInitializationArgument->arguments[argumentCount++] = INIT_EXECUTABLE_PATH;

		for (int i = 0; i < initArgc && i < ARGUMENTS_LENGTH - 2; i++) {
			const char* argument = initArgv[i];
			size_t length = strlen(argument);

			if (stringStreamWriterGetAvailable(&stringStreamWriter) >= length + 1) {
				initInitializationArgument->arguments[argumentCount++] = stringStreamWriterNextCharacterPointer(&stringStreamWriter);

				streamWriterWriteString(&stringStreamWriter.streamWriter, argument, length);
				streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');

			} else {
				break;
			}
		}

		initInitializationArgument->arguments[argumentCount++] = NULL;

		result = processManagerCreateInitProcess((void __attribute__ ((cdecl)) (*)(void*)) &initInitializationCallback,
			initInitializationArgument);
	}

	if (result != SUCCESS && doubleLinkedListElement != NULL) {
		memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
	}

	return result;
}
