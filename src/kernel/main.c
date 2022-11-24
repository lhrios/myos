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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "kernel/ata.h"
#include "kernel/busy_waiting_manager.h"
#include "kernel/command_scheduler.h"
#include "kernel/cmos.h"
#include "kernel/error_handler.h"
#include "kernel/interruption_manager.h"
#include "kernel/keyboard.h"
#include "kernel/kernel_life_cycle.h"
#include "kernel/log.h"
#include "kernel/memory_manager.h"
#include "kernel/multiboot.h"
#include "kernel/pic.h"
#include "kernel/pit.h"
#include "kernel/session_manager.h"
#include "kernel/speaker_manager.h"
#include "kernel/system_calls.h"
#include "kernel/system_call_manager.h"
#include "kernel/process/process_manager.h"
#include "kernel/tty.h"
#include "kernel/x86.h"

#include "kernel/file_system/devices_file_system.h"
#include "kernel/file_system/ext2_file_system.h"

#include "kernel/io/block_cache_manager.h"
#include "kernel/io/block_device.h"
#include "kernel/io/null_device.h"
#include "kernel/io/pipe_manager.h"
#include "kernel/io/virtual_file_system_manager.h"
#include "kernel/io/zero_device.h"

#include "kernel/process/process_group_manager.h"
#include "kernel/process/init_process_creator.h"

#include "util/command_line_utils.h"
#include "util/math_utils.h"
#include "util/scanner.h"
#include "util/string_utils.h"

#define MASTER_FIRST_INTERRUPTION_VECTOR 32 /* Master: from 32 to 39 (inclusive). */
#define SLAVE_FIRST_INTERRUPTION_VECTOR 40 /* Slave: from 40 to 47 (inclusive). */
#define PIT_INTERRUPTION_VECTOR 32 /* IRQ0. */
#define KEYBOARD_INTERRUPTION_VECTOR 33 /* IRQ1. */

static const char* DEVICE_FILE_SYSTEM_MOUNT_POINT = "/dev/";
static const char* ROOT_FILE_SYSTEM_MOUNT_POINT = "/";

static void printMemoryInformation(struct multiboot_info *multiboot_info) {
	/*
	 * If bit 0 in the ‘flags’ word is set, then the ‘mem_*’ fields are valid. ‘mem_lower’ and ‘mem_upper’ indicate
	 * the amount of lower and upper memory, respectively, in kilobytes. Lower memory starts at address 0, and upper
	 * memory starts at address 1 megabyte. The maximum possible value for lower memory is 640 kilobytes. The value
	 * returned for upper memory is maximally the address of the first upper memory hole minus 1 megabyte. It is
	 * not guaranteed to be this value.
	 */

	logDebug("Multiboot memory information:\n"
		"  mem_upper=%dMB (%d)\n"
		"  mem_lower=%dKB\n", multiboot_info->mem_upper / 1024, multiboot_info->mem_upper, multiboot_info->mem_lower);
}

static APIStatusCode mountRootFileSystem(const char* deviceFileSystemPath) {
	APIStatusCode result = SUCCESS;

	if (stringUtilsStartsWith(deviceFileSystemPath, DEVICE_FILE_SYSTEM_MOUNT_POINT)) {
		struct VirtualFileSystemNode* virtualFileSystemNode = devicesFileSystemGetDeviceVirtualFileSystemNodeByName(deviceFileSystemPath
				+ strlen(DEVICE_FILE_SYSTEM_MOUNT_POINT));
		if (virtualFileSystemNode != NULL) {
			mode_t mode = virtualFileSystemNode->operations->getMode(virtualFileSystemNode);
			if (mode & S_IS_MOUNTABLE_BLOCK_DEVICE) {
				struct BlockDeviceVirtualFileSystemNode* blockDeviceVirtualFileSystemNode = (void*) virtualFileSystemNode;
				struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
				if (doubleLinkedListElement != NULL) {
					struct Ext2FileSystem* ext2FileSystem = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
					result = ext2FileSystemInitialize(ext2FileSystem, blockDeviceVirtualFileSystemNode->blockDevice, virtualFileSystemManagerGetOpenFileDescriptionCount());
					if (result == SUCCESS) {
						virtualFileSystemManagerMountFileSystem(ROOT_FILE_SYSTEM_MOUNT_POINT, &ext2FileSystem->fileSystem);
					}

				} else {
					result = ENOMEM;
				}

			} else {
				result = ENOTBLK;
			}

		} else {
			result = ENOENT;
		}

	} else {
		result = ENOENT;
	}

	return result;
}

static bool triggerReboot(void* unused) {
	kernelLifeCycleReboot();
	return true;
}

struct CommandLineOptions {
	const char* root;
	int initialForegroundTTY;
	enum LogLevel logLevel;
	const char** initArgv;
	int initArgc;
};

#define ARGUMENTS_BUFFER_SIZE 512
#define ARGUMENTS_LENGTH 64
struct CommandLineParsingContext {
	char argumentsBuffer[ARGUMENTS_BUFFER_SIZE];
	const char* arguments[ARGUMENTS_LENGTH];
};

static void parseCommandLine(const char* rawCommandLine, struct CommandLineParsingContext* commandLineParsingContext,
		struct CommandLineOptions* commandLineOptions) {
	memset(commandLineOptions, 0, sizeof(struct CommandLineOptions));

	if (rawCommandLine != NULL) {
		int argumentsCount = 0;
		commandLineUtilsSplit(rawCommandLine, commandLineParsingContext->argumentsBuffer, ARGUMENTS_BUFFER_SIZE, commandLineParsingContext->arguments,
			ARGUMENTS_LENGTH, &argumentsCount);

		struct CommandLineArgumentsParsingConfiguration configuration;

		struct CommandLineArgumentsParsingContext commandLineArgumentsParsingContext;
		commandLineUtilsInitializeContext(&commandLineArgumentsParsingContext, 0, 1);

		struct CommandLineArgumentsParsingResult result;

		#define ROOT_OPTION_ID 2
		#define INITIAL_FOREGROUND_TTY_ID 3
		#define LOG_LEVEL_ID 4
		struct option longOptions[] = {
			{"root", required_argument, NULL, ROOT_OPTION_ID},
			{"initial-foreground-tty", required_argument, NULL, INITIAL_FOREGROUND_TTY_ID},
			{"log-level", required_argument, NULL, LOG_LEVEL_ID},
			{0, 0, 0, 0}
		};
		configuration.shortOptionCharacters = NULL;
		configuration.longOptions = longOptions;
		configuration.operationMode = HANDLE_NON_OPTIONS_AS_ARGUMENT_OF_ONE;

		bool done = false;
		while (!done) {
			commandLineUtilsParseArguments(argumentsCount, commandLineParsingContext->arguments, &configuration,
					&commandLineArgumentsParsingContext, &result);
			switch (result.errorType) {
				case COMMAND_LINE_UTILS_NO_ERROR:
					switch (result.optionValue) {
						case ROOT_OPTION_ID:
							assert(result.argument != NULL);
							commandLineOptions->root = result.argument;
							break;

						case INITIAL_FOREGROUND_TTY_ID:
							assert(result.argument != NULL);
							scannerParseInt32(result.argument, 10, true, false, NULL, &commandLineOptions->initialForegroundTTY);
							break;

						case LOG_LEVEL_ID:
						{
							enum LogLevel logLevel = logLogLevelByName(result.argument);
							if (logLevel == UNKNOWN_LOG_LEVEL) {
								logWarn("The log level \"%s\" is not known", result.argument);
							}
							commandLineOptions->logLevel = logLevel;
						} break;

						case 1:
							logWarn("Unexpected non-option: \"%s\"", result.argument);
							break;

						default:
							assert(false);
							break;
					}
					break;

				case COMMAND_LINE_UTILS_NO_MORE_ARGUMENTS:
					done = true;
					break;

				default:
					assert(false);
					break;

				case COMMAND_LINE_UTILS_MORE_THAN_ONE_OPTION_WITH_PREFIX:
					errorHandlerFatalError("The prefix \"%s\" is part of at least two options: \"%s\" and \"%s\"",
							commandLineParsingContext->arguments[commandLineArgumentsParsingContext.nextArgumentIndex - 1],
							result.option1->name, result.option2->name);
					break;

				case COMMAND_LINE_UTILS_NO_PARAMETER_ALLOWED:
					errorHandlerFatalError("The option \"%s\" does not allow a parameter but one was found",  result.option1->name);
					break;

				case COMMAND_LINE_UTILS_MISSING_REQUIRED_PARAMETER:
					errorHandlerFatalError("The option \"%s\" requires a parameter but none was found",  result.option1->name);
					break;

				case COMMAND_LINE_UTILS_UNKNOWN_OPTION:
					logWarn("Unexpected option: \"%s\"", commandLineParsingContext->arguments[commandLineArgumentsParsingContext.nextArgumentIndex - 1]);
					break;
			}
		}

		commandLineOptions->initArgv = &commandLineParsingContext->arguments[commandLineArgumentsParsingContext.nextArgumentIndex];
		commandLineOptions->initArgc = argumentsCount - commandLineArgumentsParsingContext.nextArgumentIndex;
		assert(commandLineOptions->initArgc >= 0);

	} else {
		errorHandlerFatalError("A non-null command line is required.\n");
	}
}

void __attribute__ ((cdecl)) MyOSMain(uint32_t eax, struct multiboot_info* multiboot_info) {
	/* Was the kernel loaded by a Multiboot compliant boot loader? */
	if (eax != MULTIBOOT_BOOTLOADER_MAGIC) {
		x86Ring0Stop();
	}

	/* Nothing can be written until we initialize the VGA. Therefore, it is the first module initialized. */
	ttyInitialize(multiboot_info);

	/*
	 * TODO: First thing is parse command line and store the errors. DO NOT use log.
	 * All other modules can depend on "command_line_parser" module
	 */
	struct CommandLineParsingContext commandLineParsingContext;
	struct CommandLineOptions commandLineOptions;
	if (multiboot_info != NULL && (multiboot_info->flags & MULTIBOOT_INFO_CMDLINE)) {
		parseCommandLine((void*) multiboot_info->cmdline, &commandLineParsingContext, &commandLineOptions);
	}

	logSetLogLevel(commandLineOptions.logLevel);

	cmosInitialize();

	interruptionManagerInitialize(INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL);

	picInitialize(MASTER_FIRST_INTERRUPTION_VECTOR, SLAVE_FIRST_INTERRUPTION_VECTOR);
	logDebug("PIC has been initialized");

	pitInitialize(PIT_INTERRUPTION_VECTOR);
	logDebug("PIT has been initialized");
	commandSchedulerInitialize();

	keyboardInitialize(KEYBOARD_INTERRUPTION_VECTOR);

	if (multiboot_info->flags & MULTIBOOT_MEMORY_INFO) {
		printMemoryInformation(multiboot_info);
	} else {
		errorHandlerFatalError("The boot loader has not provided the required memory information");
	}

	memoryManagerInitialize(multiboot_info->mem_upper);

	APIStatusCode result;
	uint32_t maxPageFramesToCacheBlocks = (memoryManagerGetKernelSpaceAvailablePageFrameCount() * 33) / 100; /* It uses 33% of kernel memory to cache blocks. */
	const int MIN_REQUIRED_PAGE_FRAMES_TO_CACHE_BLOCKS = 32;
	if (maxPageFramesToCacheBlocks >= MIN_REQUIRED_PAGE_FRAMES_TO_CACHE_BLOCKS) {
		result = blockCacheManagerInitialize(maxPageFramesToCacheBlocks);
		if (result != SUCCESS) {
			errorHandlerFatalError("The block cache manager could not be initialized: %s", sys_errlist[result]);
		}

	} else {
		errorHandlerFatalError("There is not enough memory to initialize block cache manager: the minimum required is %u bytes", MIN_REQUIRED_PAGE_FRAMES_TO_CACHE_BLOCKS * PAGE_FRAME_SIZE);
	}

	bool assertIsEnabled = false;
	assert(assertIsEnabled = true);
	logDebug("Assertions are %s", assertIsEnabled ? "ENABLED" : "DISABLED");

	/* The hardware will be initialized as late as possible in order to avoid spurious IRQs. */
	picInitializeHardware();
	picIssueEndOfInterrupt(IRQ0, false);
	picIssueEndOfInterrupt(IRQ1, false);
	x86Sti();
	logDebug("Enabling IRQ0");
	picEnableIRQs(IRQ0);

	/* It requires PIC and PIT in order to initialize properly. */
	busyWaitingManagerInitialize();

	if ((result = virtualFileSystemManagerInitialize()) != SUCCESS) {
		errorHandlerFatalError("The virtual file system manager could not be initialized: %s", sys_errlist[result]);
	}

	if ((result = devicesFileSystemInitialize()) != SUCCESS) {
		errorHandlerFatalError("The device file system could not be initialized: %s", sys_errlist[result]);
	}

	ataInitialize();

	/* Mount some file systems. */
	if ((result = mountRootFileSystem(commandLineOptions.root)) != SUCCESS) {
		errorHandlerFatalError("The root file system (%s) could not be mounted at \"%s\": %s", commandLineOptions.root, ROOT_FILE_SYSTEM_MOUNT_POINT, sys_errlist[result]);
	}
	if ((result = devicesFileSystemMount(DEVICE_FILE_SYSTEM_MOUNT_POINT)) != SUCCESS) {
		errorHandlerFatalError("The device file system could not be mounted at \"%s\": %s", DEVICE_FILE_SYSTEM_MOUNT_POINT, sys_errlist[result]);
	}

	nullDeviceInitialize();
	zeroDeviceInitialize();
	ttyRegisterDevices();

	if ((result = processManagerInitialize()) != SUCCESS) {
		errorHandlerFatalError("Could not initialize the process manager: %s", sys_errlist[result]);
	}

	if ((result = sessionManagerInitialize()) != SUCCESS) {
		errorHandlerFatalError("Could not initialize the session manager: %s", sys_errlist[result]);
	}

	if ((result = processGroupManagerInitialize()) != SUCCESS) {
		errorHandlerFatalError("Could not initialize the process group manager: %s", sys_errlist[result]);
	}

	if ((result = pipeManagerInitialize()) != SUCCESS) {
		errorHandlerFatalError("Could not initialize the pipe manager: %s", sys_errlist[result]);
	}

	if ((result = initProcessCreatorCreate(commandLineOptions.initArgc, commandLineOptions.initArgv)) != SUCCESS) {
		errorHandlerFatalError("Could not create the init process: %s", sys_errlist[result]);
	}

	/* It now registers some key stroke callbacks. */
	{
		bool result = keyboardRegisterKeyStroke(DELETE_KEY_SCAN_CODE, KEY_EVENT_CONTROL_KEY_PRESSED_MASK | KEY_EVENT_ALT_KEY_PRESSED_MASK, false, &triggerReboot, NULL);
		assert(result);
	}

	systemCallManagerInitialize();

	//logDebug("Testing the speaker.\n\n");
	//speakerManagerPlaySound1();

	pitStartTickGenerator(100); /* Every 10 millisecond approximately. */

	keyboardInitializeHardware();

	logDebug("Enabling IRQ1");
	picEnableIRQs(IRQ1);

	if (ttyIsValidTTYId(commandLineOptions.initialForegroundTTY)) {
		ttySetForegroundTTY(commandLineOptions.initialForegroundTTY);
	}

	keyboardSetIsIgnoringInput(false);

	/* This call never returns. */
	processManagerStartScheduling();
	assert(false);
}
