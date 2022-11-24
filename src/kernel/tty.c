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

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

#include <myos.h>

#include "kernel/api_status_code.h"
#include "kernel/cmos.h"
#include "kernel/ecma_48.h"
#include "kernel/interruption_manager.h"
#include "kernel/keyboard.h"
#include "kernel/log.h"
#include "kernel/priority.h"
#include "kernel/session_manager.h"
#include "kernel/speaker_manager.h"
#include "kernel/system_calls.h"
#include "kernel/vga.h"
#include "kernel/tty.h"

#include "kernel/file_system/devices_file_system.h"

#include "kernel/io/open_file_description.h"

#include "kernel/process/process_group.h"
#include "kernel/process/process_group_manager.h"
#include "kernel/process/process_manager.h"

#include "kernel/services/process_services.h"
#include "kernel/services/signal_services.h"

#include "util/formatter.h"
#include "util/math_utils.h"
#include "util/ring_buffer.h"
#include "util/scanner.h"
#include "util/string_utils.h"

/*
 * References:
 * - Standard ECMA-48: Control Functions for Coded Character Sets
 * - "https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Functions-using-CSI-_-ordered-by-the-final-character_s_"
 */

#define TAB_SIZE 8
#define FINE_SCROLL_DELTA 1
#define COARSE_SCROLL_DELTA 5

#define TTY_INPUT_BUFFER_CAPACITY 1024
#define TTY_MAIN_OUTPUT_BUFFER_CAPACITY (1024 * 8 * sizeof(uint16_t))
#define TTY_ALTERNATIVE_OUTPUT_BUFFER_CAPACITY (1024 * 6 * sizeof(uint16_t))

struct VirtualFileSystemOperations virtualFileSystemOperations;

struct TTYVirtualFileSystemNode {
	struct VirtualFileSystemNode virtualFileSystemNode;
	struct TTY* tty;
	mode_t mode;
};

static struct TTYVirtualFileSystemNode ttysVirtualFileSystemNodes[TTY_COUNT];

#define CONTROL_SEQUENCE_MAX_LENGTH 128

struct TTYControlSequenceState {
	uint32_t currentControlSequenceLength;
	char currentControlSequence[CONTROL_SEQUENCE_MAX_LENGTH];
	bool foundEscape;
	bool foundSquareBracket;
	bool foundQuestionMark;
};

#define DEFAULT_RENDITION_BACKGROUND_COLOR VGA_BLACK
#define DEFAULT_RENDITION_FOREGROUND_COLOR VGA_WHITE

struct TTY {
	int id;
	struct RingBuffer inputRingBuffer;
	char inputBuffer[TTY_INPUT_BUFFER_CAPACITY];
	int canonicalModeFirstColumn;
	int pendingEof;

	struct RingBuffer* currentOutputRingBuffer;

	struct RingBuffer mainOutputRingBuffer;
	char mainOutputBuffer[TTY_MAIN_OUTPUT_BUFFER_CAPACITY];

	struct RingBuffer alternativeOutputRingBuffer;
	char alternativeOutputBuffer[TTY_ALTERNATIVE_OUTPUT_BUFFER_CAPACITY];

	int* currentNextCharacterRow;
	int* currentNextCharacterColumn;
	bool* currentCursorArtificiallyOnEdgeDueToLastWrite;

	int mainNextCharacterRow;
	int mainNextCharacterColumn;
	bool mainCursorArtificiallyOnEdgeDueToLastWrite;

	int alternativeNextCharacterRow;
	int alternativeNextCharacterColumn;
	bool alternativeCursorArtificiallyOnEdgeDueToLastWrite;

	int* currentSavedNextCharacterRow;
	int* currentSavedNextCharacterColumn;

	int mainSavedNextCharacterRow;
	int mainSavedNextCharacterColumn;

	int alternativeSavedNextCharacterRow;
	int alternativeSavedNextCharacterColumn;

	uint8_t* currentForegroundColor;
	uint8_t* currentBackgroundColor;
	bool* currentInvertColors;

	uint8_t mainForegroundColor;
	uint8_t mainBackgroundColor;
	bool mainInvertColors;

	uint8_t alternativeForegroundColor;
	uint8_t alternativeBackgroundColor;
	bool alternativeInvertColors;

	int* currentScrollDelta;
	int mainScrollDelta;
	int alternativeScrollDelta;

	struct termios termiosInstance;

	bool isCursorEnabled;

	struct TTYControlSequenceState controlSequenceState;

	int totalLengthOfCompleteInputLines;

	struct DoubleLinkedList ioEventMonitoringContextList;

	struct Session* sessionBeingControlled;
	struct ProcessGroup* foregroundProcessGroup;
};

static struct TTY ttys[TTY_COUNT];
static int foregroundTTYId = 0;

static uint16_t combineCharacterAndColor(struct TTY* tty, uint8_t character) {
	if (*tty->currentInvertColors) {
		return (COMBINE_COLORS(*tty->currentBackgroundColor, *tty->currentForegroundColor) << 8) | (character & 0xFF);
	} else {
		return (COMBINE_COLORS(*tty->currentForegroundColor, *tty->currentBackgroundColor) << 8) | (character & 0xFF);
	}
}

static bool isCursorVisible(struct TTY* tty) {
	int rowCount = vgaGetRowCount();

	return (rowCount - *tty->currentNextCharacterRow) > *tty->currentScrollDelta;
}

static void doCursor(struct TTY* tty) {
	if (isCursorVisible(tty) && tty->isCursorEnabled) {
		vgaEnableCursor();
		vgaSetCursor(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow + *tty->currentScrollDelta);
	} else {
		vgaDisableCursor();
	}
}

static bool hasInputReadyToBeRead(struct TTY* tty) {
	bool canonicalMode = (tty->termiosInstance.c_lflag & ICANON) != 0;
	return (canonicalMode && (tty->pendingEof > 0 || tty->totalLengthOfCompleteInputLines > 0)) || (!canonicalMode && !ringBufferIsEmpty(&tty->inputRingBuffer));
}

static void discardData(struct TTY* tty, int selector) {
	if (selector == TCIFLUSH || selector == TCIOFLUSH) {
		tty->pendingEof = 0;
		tty->totalLengthOfCompleteInputLines = 0;
		ringBufferClear(&tty->inputRingBuffer);
	}
}

static APIStatusCode checkBackgroundProcessGroupCondition(struct TTY* tty, struct Process* currentProcess, bool onlyIfTostopIsOn, int signalId) {
	APIStatusCode result = SUCCESS;

	struct ProcessGroup* processGroup = currentProcess->processGroup;
	struct Session* processSession = processGetSession(currentProcess);

	bool isSignalIgnored = signalServicesIsSignalIgnored(currentProcess, signalId);
	bool isSignalBlocked = signalServicesIsSignalBlocked(currentProcess, signalId);

	if (tty->sessionBeingControlled == processSession && tty->foregroundProcessGroup != processGroup
			&& (!onlyIfTostopIsOn || (tty->termiosInstance.c_lflag & TOSTOP) != 0)
			&& !isSignalIgnored && !isSignalBlocked) {

		if (processGroupIsOrphaned(processGroup)) {
			result = EIO;

		} else {
			enum ResumedProcessExecutionSituation resumedProcessExecutionSituation;
			signalServicesGenerateSignal(currentProcess, -processGroup->id, signalId, false, &resumedProcessExecutionSituation);
			if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
				result = EINTR;
			}
		}
	}

	return result;
}

static APIStatusCode setForegroundProcessGroup(struct TTY* tty, struct Process* currentProcess, pid_t processGroupId) {
	APIStatusCode result = checkBackgroundProcessGroupCondition(tty, currentProcess, false, SIGTTOU);
	if (result == SUCCESS) {
		struct Session* currentProcessSession = processGetSession(currentProcess);
		struct ProcessGroup* processGroup = processGroupManagerGetAndReserveProcessGroupById(processGroupId);

		if (processGroup == NULL || processGroup->session != currentProcessSession) {
			result = EPERM;

		} else if (tty->sessionBeingControlled != processGroup->session) {
			result = ENOTTY;

		} else {
			if (result == SUCCESS) {
				if (tty->foregroundProcessGroup != NULL) {
					processGroupManagerReleaseReservation(tty->foregroundProcessGroup);
				}
				tty->foregroundProcessGroup = processGroup;
				processGroup = NULL;
			}
		}

		if (processGroup != NULL) {
			processGroupManagerReleaseReservation(processGroup);
		}
	}

	return result;
}

static void copyViewport(struct TTY* tty) {
	int columnCount = vgaGetColumnCount();
	int rowCount = vgaGetRowCount();

	uint32_t frameBufferSize = columnCount * rowCount * sizeof(uint16_t);
	uint32_t rowSize = columnCount * sizeof(uint16_t);

	uint16_t* frameBuffer = vgaGetFrameBuffer();

	assert(ringBufferSize(tty->currentOutputRingBuffer) % rowSize == 0);
	int offset = frameBufferSize + tty->mainScrollDelta * rowSize;
	ringBufferCopy(tty->currentOutputRingBuffer, frameBuffer, frameBufferSize, -offset);

	doCursor(tty);
}

static mode_t getMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	return ttyVirtualFileSystemNode->mode;
}

static APIStatusCode open(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription** openFileDescription, int flags) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	struct TTY* tty = ttyVirtualFileSystemNode->tty;

	struct Session* session = processGetSession(process);
	if (processIsSessionLeader(process) && session->controllingTTYId == -1
			&& tty->sessionBeingControlled == NULL && (flags & O_NOCTTY) == 0) {
		session->controllingTTYId = tty->id;
		tty->sessionBeingControlled = session;
	}

	return SUCCESS;
}

static APIStatusCode read(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* currentProcess, struct OpenFileDescription* openFileDescription,
		void* buffer, size_t bufferSize, size_t* count) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	struct TTY* tty = ttyVirtualFileSystemNode->tty;

	*count = 0;

	APIStatusCode result = SUCCESS;
	bool done = false;
	do {
		bool nonblocking = (openFileDescription->flags & O_NONBLOCK) != 0;

		/*
		 * When a process in a background job tries to read from its controlling terminal, the process group is usually sent a SIGTTIN signal.
		 * This normally causes all of the processes in that group to stop (unless they handle the signal and don’t stop themselves).
		 * However, if the reading process is ignoring or blocking this signal, then read fails with an EIO error instead.
		 *
		 * https://www.gnu.org/software/libc/manual/html_node/Access-to-the-Terminal.html
		 */
		result = checkBackgroundProcessGroupCondition(tty, currentProcess, false, SIGTTIN);
		if (result != SUCCESS) {
			done = true;

		} else if ((tty->termiosInstance.c_lflag & ICANON) == 0) {
			if (!ringBufferIsEmpty(&tty->inputRingBuffer)) {
				*count = mathUtilsMin(bufferSize, ringBufferSize(&tty->inputRingBuffer));
				assert(tty->totalLengthOfCompleteInputLines >= 0);
				tty->totalLengthOfCompleteInputLines -= mathUtilsMin(*count, tty->totalLengthOfCompleteInputLines);
				ringBufferRead(&tty->inputRingBuffer, buffer, bufferSize);
				done = true;

			} else if (nonblocking) {
				done = true;
				result = EAGAIN;
			}

		} else {
			/* Canonical mode: */

			if (tty->totalLengthOfCompleteInputLines > 0) {
				assert(ringBufferSize(&tty->inputRingBuffer) >= tty->totalLengthOfCompleteInputLines);
				if (tty->totalLengthOfCompleteInputLines >= bufferSize) {
					*count = bufferSize;
					ringBufferRead(&tty->inputRingBuffer, buffer, bufferSize);
					tty->totalLengthOfCompleteInputLines -= bufferSize;
					done = true;

				} else {
					char character;
					for (int i = 0; true; i++) {
						ringBufferCopy(&tty->inputRingBuffer, &character, sizeof(char), i);
						assert(tty->totalLengthOfCompleteInputLines > 0);
						tty->totalLengthOfCompleteInputLines--;
						if (character == '\n'|| tty->termiosInstance.c_cc[VEOL] == character) {
							size_t localCount = i + 1;
							ringBufferRead(&tty->inputRingBuffer, buffer, localCount);
							*count = localCount;
							done = true;
							break;
						}
					}
				}

			} else if (tty->pendingEof > 0) {
				if (!ringBufferIsEmpty(&tty->inputRingBuffer)) {
					size_t localCount = mathUtilsMin(bufferSize, ringBufferSize(&tty->inputRingBuffer));
					tty->totalLengthOfCompleteInputLines -= mathUtilsMin(localCount, tty->totalLengthOfCompleteInputLines);
					ringBufferRead(&tty->inputRingBuffer, buffer, localCount);
					*count = localCount;
				}

				tty->pendingEof--;
				done = true;

			} else if (nonblocking) {
				done = true;
				result = EAGAIN;
			}
		}

		if (!done) {
			processServicesSuspendToWaitForIO(currentProcess, &openFileDescription->virtualFileSystemNode->waitingIOProcessList, SUSPENDED_WAITING_READ);

			enum ResumedProcessExecutionSituation resumedProcessExecutionSituation = processManagerScheduleProcessExecution();
			assert(currentProcess->waitingIOProcessList == NULL);
			assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

			if (resumedProcessExecutionSituation == WILL_CALL_SIGNAL_HANDLER) {
				return EINTR;
			}
		}

	} while (!done);

	assert(!doubleLinkedListContainsFoward(&openFileDescription->virtualFileSystemNode->waitingIOProcessList, &currentProcess->waitingIOProcessListElement));
	assert(currentProcess->waitingIOProcessList == NULL);
	assert(processCountIOEventsBeingMonitored(currentProcess) == 0);

	return result;
}

static APIStatusCode status(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;

	statInstance->st_size = 0;
	statInstance->st_dev = TTY_DEVICE_ID;
	statInstance->st_ino = ttyVirtualFileSystemNode->tty->id;
	statInstance->st_atime = cmosGetInitializationTime();
	statInstance->st_ctime = cmosGetInitializationTime();
	statInstance->st_mtime = cmosGetInitializationTime();
	statInstance->st_rdev = myosCalculateUniqueId(statInstance->st_dev, statInstance->st_ino);
	statInstance->st_nlink = 1;

	return SUCCESS;
}

static APIStatusCode manipulateDeviceParameters(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* currentProcess,
		struct OpenFileDescription* openFileDescription, uint32_t* request) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	struct TTY* tty = ttyVirtualFileSystemNode->tty;

	APIStatusCode result = SUCCESS;

	if (*request == TCGETS || *request == TCSETS || *request == TCSETSW || *request == TCSETSF) {
		struct termios** termiosInstance = ((void*) request) + sizeof(void*);
		if (processIsValidSegmentAccess(currentProcess, (uint32_t) termiosInstance, sizeof(void*))
				&& processIsValidSegmentAccess(currentProcess, (uint32_t) *termiosInstance, sizeof(struct termios))) {
			if (*request == TCGETS) {
				memcpy(*termiosInstance, &tty->termiosInstance, sizeof(struct termios));

			} else {
				result = checkBackgroundProcessGroupCondition(tty, currentProcess, true, SIGTTOU);
				if (result == SUCCESS) {
					memcpy(&tty->termiosInstance, *termiosInstance, sizeof(struct termios));

					if (*request == TCSETSF) {
						discardData(tty, TCIFLUSH);
					}
				}
			}

		} else {
			result = EFAULT;
		}

	} else if (*request == TIOCGWINSZ) {
		struct winsize** winsizeInstance = ((void*) request) + sizeof(void*);
		if (processIsValidSegmentAccess(currentProcess, (uint32_t) winsizeInstance, sizeof(void*))
				&& processIsValidSegmentAccess(currentProcess, (uint32_t) *winsizeInstance, sizeof(struct winsize))) {
			(*winsizeInstance)->ws_row = vgaGetRowCount();
			(*winsizeInstance)->ws_col = vgaGetColumnCount();

		} else {
			result = EFAULT;
		}

	} else if (*request == TIOCSWINSZ) {
		result = EPERM;

	} else if (*request == TIOCSPGRP || *request == TIOCGPGRP || *request == TIOCGSID) {
		pid_t** id = ((void*) request) + sizeof(void*);
		if (processIsValidSegmentAccess(currentProcess, (uint32_t) id, sizeof(void*))
				&& processIsValidSegmentAccess(currentProcess, (uint32_t) *id, sizeof(pid_t))) {
			if (*request == TIOCGPGRP) {
				struct Session* session = processGetSession(currentProcess);
				if (session != tty->sessionBeingControlled) {
					result = ENOTTY;
				} else if (tty->foregroundProcessGroup == NULL) {
					**id = INT_MAX;
				} else {
					**id = tty->foregroundProcessGroup->id;
				}

			} else if (*request == TIOCGSID) {
				struct Session* session = processGetSession(currentProcess);
				if (session != tty->sessionBeingControlled || tty->sessionBeingControlled == NULL) {
					result = ENOTTY;
				} else {
					**id = tty->sessionBeingControlled->id;
				}

			} else if (*request == TIOCSPGRP) {
				result = setForegroundProcessGroup(tty, currentProcess, **id);

			} else {
				assert(false);
			}

		} else {
			result = EFAULT;
		}


	} else {
		result = EINVAL;
	}

	return result;
}

APIStatusCode startIoEventMonitoring(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct IOEventMonitoringContext* ioEventMonitoringContext) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	struct TTY* tty = ttyVirtualFileSystemNode->tty;

	assert(!doubleLinkedListContainsFoward(&tty->ioEventMonitoringContextList, &ioEventMonitoringContext->doubleLinkedListElement));

	short events = ioEventMonitoringContext->userIoEventMonitoringContext.events;
	if (events & POLLIN) {
		if (hasInputReadyToBeRead(tty)) {
			ioEventMonitoringContext->userIoEventMonitoringContext.revents = POLLIN;

		} else {
			doubleLinkedListInsertAfterLast(&tty->ioEventMonitoringContextList, &ioEventMonitoringContext->doubleLinkedListElement);
		}

		return SUCCESS;

	} else {
		return EINVAL;
	}
}

void stopIoEventMonitoring(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct IOEventMonitoringContext* ioEventMonitoringContext) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	struct TTY* tty = ttyVirtualFileSystemNode->tty;

	assert(doubleLinkedListContainsFoward(&tty->ioEventMonitoringContextList, &ioEventMonitoringContext->doubleLinkedListElement));
	doubleLinkedListRemove(&tty->ioEventMonitoringContextList, &ioEventMonitoringContext->doubleLinkedListElement);
}

static int calculateOffsetForRingBuffer(int column, int row) {
	int columnCount = vgaGetColumnCount();
	assert(0 <= column && column < columnCount);
	int rowCount = vgaGetRowCount();
	assert(0 <= row && row < rowCount);

	int offset = (rowCount - row) * columnCount - column;
	assert(0 < offset);
	assert(offset <= rowCount * columnCount);

	offset *= -sizeof(uint16_t);
	return offset;
}

static int calculateFrameBufferIndex(struct TTY* tty) {
	int columnCount = vgaGetColumnCount();
	return *tty->currentNextCharacterColumn + (*tty->currentNextCharacterRow + *tty->currentScrollDelta) * columnCount;
}

static void doCanonicalEraseCharacter(struct TTY* tty, char character) {
	int columnCount = vgaGetColumnCount();
	uint16_t* frameBuffer = vgaGetFrameBuffer();
	uint16_t characterAndColor = combineCharacterAndColor(tty, ' ');

	if (character == '\t') {
		int lastIncrement = -1;
		int totalIncrement = tty->canonicalModeFirstColumn;
		for (int i = 0; i < ringBufferSize(&tty->inputRingBuffer); i++) {
			char character;
			ringBufferCopy(&tty->inputRingBuffer, &character, sizeof(char), i * sizeof(char));
			if (character == '\t') {
				lastIncrement = TAB_SIZE - (totalIncrement % TAB_SIZE);
				if (totalIncrement + lastIncrement >= columnCount) {
					assert(totalIncrement < columnCount);
					lastIncrement = columnCount - 1 - totalIncrement;
					totalIncrement = columnCount - 1;
				} else {
					totalIncrement += lastIncrement;
				}

			} else if (character == '\n') {
				totalIncrement = 0;

			} else if (iscntrl(character)) {
				totalIncrement += 2;

			} else {
				totalIncrement++;
			}

			if (totalIncrement >= columnCount) {
				totalIncrement = 0;
			}
		}

		assert(lastIncrement >= 0);

		*tty->currentNextCharacterColumn -= mathUtilsMin(*tty->currentNextCharacterColumn, lastIncrement);
		*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
		doCursor(tty);

	} else {
		if (*tty->currentNextCharacterColumn > 0) {
			int columnsToErase = iscntrl(character) ? 2 : 1;
			bool isScrolling = *tty->currentScrollDelta > 0;

			while (columnsToErase > 0 && *tty->currentNextCharacterColumn > 0) {
				if (*tty->currentCursorArtificiallyOnEdgeDueToLastWrite) {
					*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
				} else {
					(*tty->currentNextCharacterColumn)--;
				}

				int offset = calculateOffsetForRingBuffer(*tty->currentNextCharacterColumn , *tty->currentNextCharacterRow);
				ringBufferOverWrite(tty->currentOutputRingBuffer, &characterAndColor, sizeof(uint16_t), offset);

				if (!isScrolling) {
					frameBuffer[calculateFrameBufferIndex(tty)] = characterAndColor;
				}
				columnsToErase--;
			}

			if (isScrolling) {
				*tty->currentScrollDelta = 0;
				copyViewport(tty);

			} else {
				doCursor(tty);
			}
		}
	}

	ringBufferDiscard(&tty->inputRingBuffer, -sizeof(char));
}

static bool doCanonicalEraseWord(struct TTY* tty) {
	struct termios* termiosInstance = &tty->termiosInstance;

	if (!ringBufferIsEmpty(&tty->inputRingBuffer)) {
		bool foundNonWhiteSpaceCharacter = false;
		while (!ringBufferIsEmpty(&tty->inputRingBuffer)) {
			char character;
			ringBufferCopy(&tty->inputRingBuffer, &character, sizeof(char), -sizeof(char));

			if (character == '\n' || ((termiosInstance->c_lflag & ICANON) != 0 && termiosInstance->c_cc[VEOL] == character)) {
				break;

			} else if (isspace(character)) {
				if (foundNonWhiteSpaceCharacter) {
					break;
				} else {
					doCanonicalEraseCharacter(tty, character);
				}

			} else {
				foundNonWhiteSpaceCharacter = true;
				doCanonicalEraseCharacter(tty, character);
			}
		}

	} else if (*tty->currentScrollDelta > 0) {
		*tty->currentScrollDelta = 0;
		copyViewport(tty);
	}

	return false;
}

static bool doCanonicalBackspace(struct TTY* tty) {
	bool echo = false;
	struct termios* termiosInstance = &tty->termiosInstance;

	if (!ringBufferIsEmpty(&tty->inputRingBuffer)) {
		char character;
		ringBufferCopy(&tty->inputRingBuffer, &character, sizeof(char), -sizeof(char));
		if (character != '\n' && termiosInstance->c_cc[VEOL] != character && tty->pendingEof == 0) {
			if ((termiosInstance->c_lflag & ECHO) != 0 && (termiosInstance->c_lflag & ECHOE) != 0) {
				doCanonicalEraseCharacter(tty, character);

			} else {
				ringBufferDiscard(&tty->inputRingBuffer, -sizeof(char));
				echo = (termiosInstance->c_lflag & ECHO) != 0;
			}
		}

	} else if (*tty->currentScrollDelta > 0) {
		*tty->currentScrollDelta = 0;
		copyViewport(tty);
	}

	return echo;
}

static int calculateNextCharacterOffsetForRingBuffer(struct TTY* tty) {
	return calculateOffsetForRingBuffer(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow);
}

static void resetScroll(struct TTY* tty) {
	if (tty->mainScrollDelta != 0) {
		tty->mainScrollDelta = 0;
		copyViewport(tty);
	}
}

static void writeToTTYOutput(struct TTY* tty, uint8_t character, bool resetScroll) {
	struct TTYControlSequenceState* controlSequenceState = &tty->controlSequenceState;

	int columnCount = vgaGetColumnCount();
	int rowCount = vgaGetRowCount();
	int rowSize = columnCount * sizeof(uint16_t);
	uint16_t* frameBuffer = vgaGetFrameBuffer();

	int ttyId = tty->id;
	uint16_t characterAndColor = combineCharacterAndColor(tty, ' ');
	bool writeOnRingBufferOnly = !isCursorVisible(tty) || ttyId != foregroundTTYId;

	if (*(tty->currentCursorArtificiallyOnEdgeDueToLastWrite)) {
		*tty->currentNextCharacterColumn = 0;
		(*tty->currentNextCharacterRow)++;

		if (*tty->currentNextCharacterRow >= rowCount) {
			characterAndColor = combineCharacterAndColor(tty, ' ');
			for (int column = 0; column < columnCount; column++) {
				ringBufferWrite(tty->currentOutputRingBuffer, &characterAndColor, sizeof(uint16_t));
			}
			assert(ringBufferSize(tty->currentOutputRingBuffer) % rowSize == 0);
			*tty->currentNextCharacterRow = rowCount - 1;

			writeOnRingBufferOnly = true;
		}

		*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
	}
	assert(0 <= *tty->currentNextCharacterColumn && *tty->currentNextCharacterColumn < columnCount);
	assert(0 <= *tty->currentNextCharacterRow && *tty->currentNextCharacterRow < rowCount);

	bool processedAsControlCharacter;
	if (!controlSequenceState->foundEscape) {
		switch(character) {
			case '\f':
			case '\n':
				writeOnRingBufferOnly = writeOnRingBufferOnly || *tty->currentNextCharacterRow + 1 >= rowCount;

				if (character == '\n') {
					*tty->currentNextCharacterColumn = 0;
				}
				(*tty->currentNextCharacterRow)++;

				if (*tty->currentNextCharacterRow >= rowCount) {
					for (int column = 0; column < columnCount; column++) {
						ringBufferWrite(tty->currentOutputRingBuffer, &characterAndColor, sizeof(uint16_t));
					}
					assert(ringBufferSize(tty->currentOutputRingBuffer) % rowSize == 0);
					*tty->currentNextCharacterRow = rowCount - 1;
				}

				*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
				processedAsControlCharacter = true;
			break;

			case '\t':
			{
				int increment = TAB_SIZE - (*tty->currentNextCharacterColumn % TAB_SIZE);
				*tty->currentNextCharacterColumn += mathUtilsMin(columnCount - *tty->currentNextCharacterColumn - 1, increment);
				*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
				processedAsControlCharacter = true;
			}
			break;

			case '\r':
				*tty->currentNextCharacterColumn = 0;
				*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
				processedAsControlCharacter = true;
			break;

			case '\b':
				*tty->currentNextCharacterColumn -= mathUtilsMin(1, *tty->currentNextCharacterColumn);
				*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
				processedAsControlCharacter = true;
			break;

			case '\a':
				speakerManagerBeep(1);
				processedAsControlCharacter = true;
			break;

			default:
				processedAsControlCharacter = false;
			break;

		}

	} else {
		processedAsControlCharacter = false;
	}

	if (!processedAsControlCharacter) {
		writeOnRingBufferOnly = writeOnRingBufferOnly || *tty->currentNextCharacterColumn + 1 >= columnCount;

		characterAndColor = combineCharacterAndColor(tty, character);
		int offset = calculateNextCharacterOffsetForRingBuffer(tty);
		ringBufferOverWrite(tty->currentOutputRingBuffer, &characterAndColor, sizeof(uint16_t), offset);
		assert(ringBufferSize(tty->currentOutputRingBuffer) % rowSize == 0);

		if (!writeOnRingBufferOnly) {
			frameBuffer[calculateFrameBufferIndex(tty)] = characterAndColor;
		}

		if (*tty->currentNextCharacterColumn + 1 >= columnCount) {
			*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = true;
		} else {
			(*tty->currentNextCharacterColumn)++;
		}
	}

	assert(0 <= *tty->currentNextCharacterColumn && *tty->currentNextCharacterColumn < columnCount);
	assert(0 <= *tty->currentNextCharacterRow && *tty->currentNextCharacterRow < rowCount);

	if (ttyId == foregroundTTYId) {
		if (writeOnRingBufferOnly) {
			if (resetScroll) {
				*tty->currentScrollDelta = 0;
			}
			copyViewport(tty);

		} else {
			if (resetScroll && *tty->currentScrollDelta != 0) {
				*tty->currentScrollDelta = 0;
				copyViewport(tty);

			} else {
				doCursor(tty);
			}
		}
	}
}

static void doControlSequenceSelectGraphicRenditionAspect(struct TTY* tty, int parameter) {
	switch (parameter) {
		case 0:
			*tty->currentBackgroundColor = DEFAULT_RENDITION_BACKGROUND_COLOR;
			*tty->currentForegroundColor = DEFAULT_RENDITION_FOREGROUND_COLOR;
			*tty->currentInvertColors = false;
			break;

		case 7:
			*tty->currentInvertColors = true;
			break;

		/* Foreground: */
		case 30:
			*tty->currentForegroundColor = VGA_BLACK;
			break;
		case 31:
			*tty->currentForegroundColor = VGA_RED;
			break;
		case 32:
			*tty->currentForegroundColor = VGA_GREEN;
			break;
		case 33:
			*tty->currentForegroundColor = VGA_YELLOW;
			break;
		case 34:
			*tty->currentForegroundColor = VGA_BLUE;
			break;
		case 35:
			*tty->currentForegroundColor = VGA_MAGENTA;
			break;
		case 36:
			*tty->currentForegroundColor = VGA_CYAN;
			break;
		case 37:
			*tty->currentForegroundColor = VGA_WHITE;
			break;
		case 90:
			*tty->currentForegroundColor = VGA_BRIGHT_BLACK;
			break;
		case 91:
			*tty->currentForegroundColor = VGA_BRIGHT_RED;
			break;
		case 92:
			*tty->currentForegroundColor = VGA_BRIGHT_GREEN;
			break;
		case 93:
			*tty->currentForegroundColor = VGA_BRIGHT_YELLOW;
			break;
		case 94:
			*tty->currentForegroundColor = VGA_BRIGHT_BLUE;
			break;
		case 95:
			*tty->currentForegroundColor = VGA_BRIGHT_MAGENTA;
			break;
		case 96:
			*tty->currentForegroundColor = VGA_BRIGHT_CYAN;
			break;
		case 97:
			*tty->currentForegroundColor = VGA_BRIGHT_WHITE;
			break;

		/* Background: */
		case 40:
			*tty->currentBackgroundColor = VGA_BLACK;
			break;
		case 41:
			*tty->currentBackgroundColor = VGA_RED;
			break;
		case 42:
			*tty->currentBackgroundColor = VGA_GREEN;
			break;
		case 43:
			*tty->currentBackgroundColor = VGA_YELLOW;
			break;
		case 44:
			*tty->currentBackgroundColor = VGA_BLUE;
			break;
		case 45:
			*tty->currentBackgroundColor = VGA_MAGENTA;
			break;
		case 46:
			*tty->currentBackgroundColor = VGA_CYAN;
			break;
		case 47:
			*tty->currentBackgroundColor = VGA_WHITE;
			break;
		case 100:
			*tty->currentBackgroundColor = VGA_BRIGHT_BLACK;
			break;
		case 101:
			*tty->currentBackgroundColor = VGA_BRIGHT_RED;
			break;
		case 102:
			*tty->currentBackgroundColor = VGA_BRIGHT_GREEN;
			break;
		case 103:
			*tty->currentBackgroundColor = VGA_BRIGHT_YELLOW;
			break;
		case 104:
			*tty->currentBackgroundColor = VGA_BRIGHT_BLUE;
			break;
		case 105:
			*tty->currentBackgroundColor = VGA_BRIGHT_MAGENTA;
			break;
		case 106:
			*tty->currentBackgroundColor = VGA_BRIGHT_CYAN;
			break;
		case 107:
			*tty->currentBackgroundColor = VGA_BRIGHT_WHITE;
			break;
	}
}

/*
 * CUP - Cursor position:
 */
static void doControlSequenceCursorPosition(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	int row = 1;
	int column = 1;
	const char* separatorBegin = strchr((const char*) ttyControlSequenceState->currentControlSequence, ';');
	if (separatorBegin != NULL) {
		int rowStringLength = separatorBegin - (const char*) ttyControlSequenceState->currentControlSequence;
		assert(rowStringLength >= 0);
		int columnStringLength = (const char*) &ttyControlSequenceState->currentControlSequence[ttyControlSequenceState->currentControlSequenceLength] - (separatorBegin + 1);
		assert(columnStringLength >= 0);

		if (rowStringLength > 0 && columnStringLength > 0) {
			scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &row);
			scannerParseInt32(separatorBegin + 1, 10, false, false, NULL, &column);
			row = mathUtilsClampInt32(row, 1, vgaGetRowCount());
			column = mathUtilsClampInt32(column, 1, vgaGetColumnCount());
		}
	}

	row--;
	column--;
	*tty->currentNextCharacterRow = row;
	*tty->currentNextCharacterColumn = column;
	*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;

	if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
		vgaSetCursor(column, row);
	}
}

/*
 * SGR - Select graphic rendition
 */
static void doControlSequenceSelectGraphicRendition(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	if (ttyControlSequenceState->currentControlSequenceLength == 0) {
		/* Default: */
		doControlSequenceSelectGraphicRenditionAspect(tty, 0);

	} else {
		int parameterBeginIndex = 0;
		for (int parameterEndIndex = 0; parameterEndIndex <= ttyControlSequenceState->currentControlSequenceLength; parameterEndIndex++) {
			if (ttyControlSequenceState->currentControlSequence[parameterEndIndex] == ';'
					|| parameterEndIndex == ttyControlSequenceState->currentControlSequenceLength) {

				if (stringUtilsNIsDigitOnly((char*) &ttyControlSequenceState->currentControlSequence[parameterBeginIndex], parameterEndIndex - parameterBeginIndex)) {
					int parameter;
					scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence[parameterBeginIndex], 10, false, false, NULL, &parameter);

					doControlSequenceSelectGraphicRenditionAspect(tty, parameter);
				}

				parameterBeginIndex = parameterEndIndex + 1;
			}
		}
	}
}

/*
 * ED - Erase in display
 */
static void doControlSequenceEraseInDisplay(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	int columnCount = vgaGetColumnCount();
	int rowCount = vgaGetRowCount();

	int parameter = 0;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	uint16_t characterAndColor = combineCharacterAndColor(tty, ' ');

	if (parameter != 3) {
		int firstColumn;
		int firstRow;
		int lastColumn;
		int lastRow;
		switch (parameter) {
			default:
			case 0:
				firstColumn = *tty->currentNextCharacterColumn;
				firstRow = *tty->currentNextCharacterRow;
				lastColumn = columnCount - 1;
				lastRow = rowCount - 1;
				break;

			case 1:
				firstColumn = 0;
				firstRow = 0;
				lastColumn = *tty->currentNextCharacterColumn;
				lastRow = *tty->currentNextCharacterRow;
				break;

			case 2:
				firstColumn = 0;
				firstRow = 0;
				lastColumn = columnCount - 1;
				lastRow = rowCount - 1;
				break;
		}

		for (int row = firstRow; row <= lastRow; row++) {
			for (int column = firstColumn; column <= (row == lastRow ? lastColumn : columnCount - 1); column++) {
				int offset = calculateOffsetForRingBuffer(column, row);
				ringBufferOverWrite(tty->currentOutputRingBuffer, &characterAndColor, sizeof(uint16_t), offset);
			}
		}

		if (tty->id == foregroundTTYId
				/* Is there any intersection with what is being displayed right now? */
				&& *tty->currentScrollDelta < rowCount) {
			copyViewport(tty);
		}

	} else if (ringBufferSize(tty->currentOutputRingBuffer) > rowCount * columnCount * sizeof(uint16_t)) {
		ringBufferDiscard(tty->currentOutputRingBuffer, ringBufferSize(tty->currentOutputRingBuffer) - rowCount * columnCount * sizeof(uint16_t));
		assert(ringBufferSize(tty->currentOutputRingBuffer) == rowCount * columnCount * sizeof(uint16_t));
		if (*tty->currentScrollDelta > 0) {
			*tty->currentScrollDelta = 0;
			if (tty->id == foregroundTTYId) {
				copyViewport(tty);
			}
		}
	}
}

static void eraseLine(struct TTY* tty, int firstColumn, int lastColumn) {
	uint16_t* frameBuffer = vgaGetFrameBuffer();
	uint16_t characterAndColor = combineCharacterAndColor(tty, ' ');

	bool isCursorVisibleResult = isCursorVisible(tty);
	for (int column = firstColumn; column <= lastColumn; column++) {
		int offset = calculateOffsetForRingBuffer(column, *tty->currentNextCharacterRow);
		ringBufferOverWrite(tty->currentOutputRingBuffer, &characterAndColor, sizeof(uint16_t), offset);
		if (isCursorVisibleResult && tty->id == foregroundTTYId) {
			frameBuffer[calculateFrameBufferIndex(tty) - *tty->currentNextCharacterColumn + column] = characterAndColor;
		}
	}
}

/*
 * EL - Erase in line
 */
static void doControlSequenceEraseInLine(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;
	int columnCount = vgaGetColumnCount();
	int parameter = 0;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	int firstColumn;
	int lastColumn;
	switch (parameter) {
		default:
		case 0:
			firstColumn = *tty->currentNextCharacterColumn;
			lastColumn = columnCount - 1;
			break;

		case 1:
			firstColumn = 0;
			lastColumn = *tty->currentNextCharacterColumn;
			break;

		case 2:
			firstColumn = 0;
			lastColumn = columnCount - 1;
			break;
	}

	eraseLine(tty, firstColumn, lastColumn);
}

/*
 * CUB - Cursor Backward
 */
static void doControlSequenceCursorBackward(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	int parameter = 1;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	*tty->currentNextCharacterColumn -= mathUtilsMin(*tty->currentNextCharacterColumn, parameter);
	*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;

	if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
		vgaSetCursor(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow);
	}
}

/*
 * CUU - Cursor Up
 */
static void doControlSequenceCursorUp(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	int parameter = 1;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	*tty->currentNextCharacterRow -= mathUtilsMin(*tty->currentNextCharacterRow, parameter);
	*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;

	if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
		vgaSetCursor(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow);
	}
}

/*
 * CUD - Cursor Down
 */
static void doControlSequenceCursorDown(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;
	int rowCount = vgaGetRowCount();
	int parameter = 1;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	*tty->currentNextCharacterRow += mathUtilsMin(rowCount - *tty->currentNextCharacterRow - 1, parameter);
	*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;

	if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
		vgaSetCursor(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow);
	}
}

/*
 * CUF - Cursor Forward
 */
static void doControlSequenceCursorForward(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;
	int columnCount = vgaGetColumnCount();
	int parameter = 1;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	*tty->currentNextCharacterColumn += mathUtilsMin(columnCount - *tty->currentNextCharacterColumn - 1, parameter);
	*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;

	if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
		vgaSetCursor(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow);
	}
}

static void doControlSequenceHideCursor(struct TTY* tty) {
	if (tty->isCursorEnabled) {
		tty->isCursorEnabled = false;
		if (tty->id == foregroundTTYId) {
			doCursor(tty);
		}
	}
}

static void doControlSequenceShowCursor(struct TTY* tty) {
	if (!tty->isCursorEnabled) {
		tty->isCursorEnabled = true;
		if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
			doCursor(tty);
		}
	}
}

static void doControlSequenceSwitchToAlternativeScreenBuffer(struct TTY* tty) {
	if (tty->currentOutputRingBuffer == &tty->mainOutputRingBuffer) {
		tty->alternativeNextCharacterColumn = 0;
		tty->alternativeNextCharacterRow = 0;
		tty->alternativeScrollDelta = 0;
		tty->alternativeBackgroundColor = DEFAULT_RENDITION_BACKGROUND_COLOR;
		tty->alternativeForegroundColor = DEFAULT_RENDITION_FOREGROUND_COLOR;

		tty->currentOutputRingBuffer = &tty->alternativeOutputRingBuffer;
		tty->currentNextCharacterColumn = &tty->alternativeNextCharacterColumn;
		tty->currentNextCharacterRow = &tty->alternativeNextCharacterRow;
		tty->currentCursorArtificiallyOnEdgeDueToLastWrite = &tty->alternativeCursorArtificiallyOnEdgeDueToLastWrite;
		tty->currentScrollDelta = &tty->alternativeScrollDelta;
		tty->currentInvertColors = &tty->alternativeInvertColors;
		tty->currentBackgroundColor = &tty->alternativeBackgroundColor;
		tty->currentForegroundColor = &tty->alternativeForegroundColor;
		tty->currentSavedNextCharacterColumn = &tty->alternativeSavedNextCharacterColumn;
		tty->currentSavedNextCharacterRow = &tty->alternativeSavedNextCharacterRow;

		if (tty->id == foregroundTTYId) {
			copyViewport(tty);
		}
	}
}

static void doControlSequenceSwitchFromAlternativeScreenBuffer(struct TTY* tty) {
	if (tty->currentOutputRingBuffer == &tty->alternativeOutputRingBuffer) {
		tty->mainScrollDelta = 0;

		tty->currentOutputRingBuffer = &tty->mainOutputRingBuffer;
		tty->currentNextCharacterColumn = &tty->mainNextCharacterColumn;
		tty->currentNextCharacterRow = &tty->mainNextCharacterRow;
		tty->currentCursorArtificiallyOnEdgeDueToLastWrite = &tty->mainCursorArtificiallyOnEdgeDueToLastWrite;
		tty->currentScrollDelta = &tty->mainScrollDelta;
		tty->currentInvertColors = &tty->mainInvertColors;
		tty->currentBackgroundColor = &tty->mainBackgroundColor;
		tty->currentForegroundColor = &tty->mainForegroundColor;
		tty->currentSavedNextCharacterColumn = &tty->mainSavedNextCharacterColumn;
		tty->currentSavedNextCharacterRow = &tty->mainSavedNextCharacterRow;

		if (tty->id == foregroundTTYId) {
			copyViewport(tty);
		}
	}
}

static void doControlSequenceSaveCursorPosition(struct TTY* tty) {
	*tty->currentSavedNextCharacterColumn = *tty->currentNextCharacterColumn;
	*tty->currentSavedNextCharacterRow = *tty->currentNextCharacterRow;
}

static void doControlSequenceRestoreCursorPosition(struct TTY* tty) {
	*tty->currentNextCharacterColumn = *tty->currentSavedNextCharacterColumn;
	*tty->currentNextCharacterRow = *tty->currentSavedNextCharacterRow;

	if (tty->id == foregroundTTYId && isCursorVisible(tty)) {
		vgaSetCursor(*tty->currentNextCharacterColumn, *tty->currentNextCharacterRow);
	}
}

/* DSR - Device Status Report */
static void doControlSequenceDeviceStatusReport(struct TTY* tty) {
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	int parameter = 0;

	if (ttyControlSequenceState->currentControlSequenceLength > 0) {
		scannerParseInt32((char*) &ttyControlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);
	}

	if (parameter == 6) {
		char buffer[32];
		sprintf(buffer, "\x1B[%d;%dR", *tty->currentNextCharacterRow + 1, *tty->currentNextCharacterColumn + 1);

		size_t answerLength = strlen(buffer);
		if (ringBufferRemaining(&tty->inputRingBuffer) >= answerLength) {
			ringBufferWrite(&tty->inputRingBuffer, buffer, answerLength);
		}
	}
}

static bool doControlSequence(struct TTY* tty, char finalCharacter) {
	struct TTYControlSequenceState* controlSequenceState = &tty->controlSequenceState;

	if (finalCharacter == '6') {
		doControlSequenceSaveCursorPosition(tty);
		return true;

	} else if (finalCharacter == '7') {
		doControlSequenceRestoreCursorPosition(tty);
		return true;

	} else if (controlSequenceState->foundQuestionMark) {
		switch (finalCharacter) {
			case 'h':
			case 'l':
			{

				if (controlSequenceState->currentControlSequenceLength > 0) {
					int parameter = 0;
					scannerParseInt32(controlSequenceState->currentControlSequence, 10, false, false, NULL, &parameter);

					if (parameter == 25) {
						if (finalCharacter == 'l') {
							doControlSequenceHideCursor(tty);
						} else {
							doControlSequenceShowCursor(tty);
						}

					} else if (parameter == 1049) {
						if (finalCharacter == 'h') {
							doControlSequenceSwitchToAlternativeScreenBuffer(tty);
						} else {
							doControlSequenceSwitchFromAlternativeScreenBuffer(tty);
						}
					}
				}

			} break;

			default:
				return false;
				break;
		}

	} else {
		switch (finalCharacter) {
			case 's':
				doControlSequenceSaveCursorPosition(tty);
				break;

			case 'u':
				doControlSequenceRestoreCursorPosition(tty);
				break;

			case 'm':
				doControlSequenceSelectGraphicRendition(tty);
				break;

			case 'n':
				doControlSequenceDeviceStatusReport(tty);
				break;

			case 'j':
			case 'H':
				doControlSequenceCursorPosition(tty);
				break;

			case 'K':
				doControlSequenceEraseInLine(tty);
				break;

			case 'J':
				doControlSequenceEraseInDisplay(tty);
				break;

			case 'D':
				doControlSequenceCursorBackward(tty);
				break;

			case 'C':
				doControlSequenceCursorForward(tty);
				break;

			case 'A':
				doControlSequenceCursorUp(tty);
				break;

			case 'B':
				doControlSequenceCursorDown(tty);
				break;

			default:
				return false;
		}
	}

	return true;
}

void writeToOutput(int ttyId, char character, bool isEcho) {
	struct TTY* tty = &ttys[ttyId];
	struct TTYControlSequenceState* ttyControlSequenceState = &tty->controlSequenceState;

	if (!ttyControlSequenceState->foundEscape && !ttyControlSequenceState->foundSquareBracket) {
		if (character == '\x1B') {
			memset(ttyControlSequenceState, 0, sizeof(struct TTYControlSequenceState));
			ttyControlSequenceState->foundEscape = true;

		} else {
			writeToTTYOutput(tty, character, isEcho);
		}

	} else if (ttyControlSequenceState->foundEscape && !ttyControlSequenceState->foundSquareBracket) {
		if (character == '[') {
			ttyControlSequenceState->foundSquareBracket = true;

		} else {
			if (!doControlSequence(tty, character)) {
				writeToTTYOutput(tty, character, isEcho);
			}
			ttyControlSequenceState->foundEscape = false;
		}

	} else if (ttyControlSequenceState->foundEscape && ttyControlSequenceState->foundSquareBracket) {
		if (!ttyControlSequenceState->foundQuestionMark && character == '?') {
			ttyControlSequenceState->foundQuestionMark = true;

		} else if (ecma48IsControlSequenceParameterCharacter(character)) {
			if (ttyControlSequenceState->currentControlSequenceLength + 2 >= CONTROL_SEQUENCE_MAX_LENGTH) {
				ttyControlSequenceState->foundEscape = false;
				writeToTTYOutput(tty, character, isEcho);

			} else {
				ttyControlSequenceState->currentControlSequence[ttyControlSequenceState->currentControlSequenceLength++] = character;
			}

		} else if (ecma48IsControlSequenceFinalCharacter(character)) {
			assert(ttyControlSequenceState->currentControlSequenceLength + 1 < CONTROL_SEQUENCE_MAX_LENGTH);
			ttyControlSequenceState->currentControlSequence[ttyControlSequenceState->currentControlSequenceLength + 1] = '\0';

			doControlSequence(tty, character);
			memset(ttyControlSequenceState, 0, sizeof(struct TTYControlSequenceState));
		}

	} else {
		writeToTTYOutput(tty, character, isEcho);
	}
}

static APIStatusCode write(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* currentProcess,
		struct OpenFileDescription* openFileDescription, void* buffer, size_t bufferSize, size_t* count) {
	struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = (struct TTYVirtualFileSystemNode*) virtualFileSystemNode;
	struct TTY* tty = ttyVirtualFileSystemNode->tty;

	APIStatusCode result = SUCCESS;
	bool done = false;
	do {
		/*
		 * Similarly, when a process in a background job tries to write to its controlling terminal, the default behavior is to send a SIGTTOU
		 * signal to the process group. However, the behavior is modified by the TOSTOP bit of the local modes flags (see Local Modes).
		 * If this bit is not set (which is the default), then writing to the controlling terminal is always permitted without sending a signal.
		 * Writing is also permitted if the SIGTTOU signal is being ignored or blocked by the writing process.
		 *
		 * https://www.gnu.org/software/libc/manual/html_node/Access-to-the-Terminal.html
		 */
		result = checkBackgroundProcessGroupCondition(tty, currentProcess, true, SIGTTOU);

		if (result != SUCCESS) {
			done = true;

		} else {
			uint8_t* castedBuffer = buffer;
			for (size_t i = 0; i < bufferSize; i++) {
				writeToOutput(tty->id, castedBuffer[i], false);
			}
			*count = bufferSize;
			done = true;
		}

	} while (!done);

	return result;
}

static void writeToTTYInput(struct TTY* tty, char* characters, uint32_t characterCount, bool eol) {
	if (ringBufferRemaining(&tty->inputRingBuffer) >= characterCount) {
		struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = &ttysVirtualFileSystemNodes[tty->id];

		if (ringBufferIsEmpty(&tty->inputRingBuffer) && (tty->termiosInstance.c_lflag & ICANON) != 0) {
			tty->canonicalModeFirstColumn = *tty->currentNextCharacterColumn;
		}

		ringBufferWrite(&tty->inputRingBuffer, characters, characterCount);

		if (eol) {
			tty->totalLengthOfCompleteInputLines = ringBufferSize(&tty->inputRingBuffer);
		}

		if (hasInputReadyToBeRead(tty)) {
			/* Are there any process waiting? Wake up them! */
			processServicesWakeUpProcesses(processManagerGetCurrentProcess(), &ttyVirtualFileSystemNode->virtualFileSystemNode.waitingIOProcessList, SUSPENDED_WAITING_READ);

			// TODO: Might be useful in other parts of the code when there is more support to poll operation
			struct DoubleLinkedList* list = &tty->ioEventMonitoringContextList;
			struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(list);
			while (listElement != NULL) {
				struct IOEventMonitoringContext* ioEventMonitoringContext = (void*) listElement;
				listElement = listElement->next;

				if (ioEventMonitoringContext->isBeingMonitored) {
					assert(ioEventMonitoringContext->userIoEventMonitoringContext.events & POLLIN);
					assert(ioEventMonitoringContext->process->state == SUSPENDED_WAITING_IO_EVENT);

					ioEventMonitoringContext->userIoEventMonitoringContext.revents = POLLIN;
					processManagerChangeProcessState(processManagerGetCurrentProcess(), ioEventMonitoringContext->process, RUNNABLE, 0);
					/* Stop all monitoring related to this process as it has been waken up. */
					processStopMonitoringIOEvents(ioEventMonitoringContext->process);
				}
			}
		}

	} else {
		speakerManagerBeep(1);
	}
}

static bool doEcho(struct TTY* tty, KeyEvent keyEvent, bool isASCIICharacter, char* characters, int charactersCount) {
	struct termios* termiosInstance = &tty->termiosInstance;
	if (termiosInstance->c_lflag & ECHO) {
		if (charactersCount == 1) {
			char character = characters[0];
			if (iscntrl(character) && (termiosInstance->c_lflag & ECHOCTL) != 0
					&& character != '\n' && character != '\t'
					// TODO: Implement me!
					// character != termiosInstance.c_cc[VSTART]
					// character != termiosInstance.c_cc[VSTOP]
			) {
				writeToTTYOutput(tty, '^', true);
				if (character == 0x7F) {
					writeToTTYOutput(tty, '?', true);

				} else {
					writeToTTYOutput(tty, character + '@', true);
				}
				return true;

			} else if (isASCIICharacter) {
				writeToTTYOutput(tty, character, true);
				return true;
			}

		} else {
			for (int i = 0; i < charactersCount; i++) {
				char character = characters[i];
				if (character == '\x1B') {
					writeToTTYOutput(tty, '^', true);
					writeToTTYOutput(tty, '[', true);
				} else {
					writeToTTYOutput(tty, character, true);
				}
			}
		}

	} else if ((termiosInstance->c_lflag & ECHONL) != 0 && charactersCount == 1 && characters[0] == '\n') {
		writeToTTYOutput(tty, '\n', true);
		return true;
	}

	return false;
}

struct SignalGenerationConfiguration {
	struct ProcessGroup* processGroup;
	int signalId;
};
struct SignalGenerationConfiguration signalGenerationConfiguration;
static void generateSignalDueToTTYInteraction(void) {
	signalServicesGenerateSignal(processManagerGetCurrentProcess(), -signalGenerationConfiguration.processGroup->id, signalGenerationConfiguration.signalId, false, NULL);
}

static bool handleSpecialInputSequences(struct TTY* tty, char* characters, int characterCount, bool* echo) {
	int columnCount = vgaGetColumnCount();
	struct Session* session = tty->sessionBeingControlled;
	struct ProcessGroup* processGroup = tty->foregroundProcessGroup;

	*echo = false;

	if (characterCount == 1) {
		char character = characters[0];

		if (session != NULL && processGroup != NULL && (tty->termiosInstance.c_lflag & ISIG) != 0
				&& (tty->termiosInstance.c_cc[VSUSP] == character || tty->termiosInstance.c_cc[VQUIT] == character || tty->termiosInstance.c_cc[VINTR] == character)) {
			*echo = true;
			int signalId = 0;

			if (tty->termiosInstance.c_cc[VSUSP] == character) {
				signalId = SIGTSTP;

			} else if (tty->termiosInstance.c_cc[VQUIT] == character) {
				signalId = SIGQUIT;

			} else if (tty->termiosInstance.c_cc[VINTR] == character) {
				signalId = SIGINT;

			} else {
				assert(false);
			}

			if ((tty->termiosInstance.c_lflag & NOFLSH) == 0) {
				discardData(tty, TCIOFLUSH);
			}

			signalGenerationConfiguration.processGroup = processGroup;
			signalGenerationConfiguration.signalId = signalId;
			interruptionManagerRegisterCommandToRunAfterInterruptionHandler(PRIORITY_HIGH, (void (*)(void*)) &generateSignalDueToTTYInteraction, NULL);

			return true;
		}

		if ((tty->termiosInstance.c_lflag & ICANON) != 0) {
			if (tty->termiosInstance.c_cc[VKILL] == character) {
				if ((tty->termiosInstance.c_lflag & ECHOK) != 0) {
					if (!ringBufferIsEmpty(&tty->inputRingBuffer)) {
						int firstColumn;
						int lastColumn = *tty->currentNextCharacterColumn;
						if (tty->canonicalModeFirstColumn + ringBufferSize(&tty->inputRingBuffer) >= columnCount
								|| tty->canonicalModeFirstColumn >= *tty->currentNextCharacterColumn) {
							firstColumn = 0;
						} else {
							firstColumn = tty->canonicalModeFirstColumn;
						}
						*tty->currentNextCharacterColumn = firstColumn;
						*tty->currentCursorArtificiallyOnEdgeDueToLastWrite = false;
						eraseLine(tty, firstColumn, lastColumn);
						doCursor(tty);
						ringBufferClear(&tty->inputRingBuffer);
					}

				} else {
					*echo = !ringBufferIsEmpty(&tty->inputRingBuffer);
					ringBufferClear(&tty->inputRingBuffer);
				}
				return true;

			} else if (tty->termiosInstance.c_cc[VEOF] == character) {
				struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = &ttysVirtualFileSystemNodes[tty->id];

				assert(tty->pendingEof >= 0);
				tty->pendingEof++;

				processServicesWakeUpProcesses(processManagerGetCurrentProcess(), &ttyVirtualFileSystemNode->virtualFileSystemNode.waitingIOProcessList, SUSPENDED_WAITING_READ);

				return true;

			} else if (tty->termiosInstance.c_cc[VEOL] == character) {
				writeToTTYInput(tty, characters, characterCount, true);
				*echo = true;
				return true;

			} else if (character == '\a') {
				speakerManagerBeep(1);
				return true;
			}
		}
	}

	return false;
}

static char transformCharAccordingToFlags(struct TTY* tty, char character) {
	if (character == '\r') {
		if ((tty->termiosInstance.c_iflag & IGNCR) == 0) {
			if ((tty->termiosInstance.c_iflag & ICRNL) != 0) {
				return '\n';
			} else {
				return '\r';
			}

		} else {
			return '\0';
		}

	} else if (character == '\n') {
		if ((tty->termiosInstance.c_iflag & INLCR) != 0) {
			return '\r';
		} else {
			return '\n';
		}

	} else {
		return character;
	}
}

static void handleKeyEventWithControlKeyPressed(struct TTY* tty, uint32_t scanCode, bool isASCIICharacter, char character, char* characters, int* charactersCount) {
	if (isASCIICharacter) {
		character = toupper(character);

		if ('@' <= character && character <= '_') {
			character = transformCharAccordingToFlags(tty, character - '@');
			if (character != '\0') {
				characters[(*charactersCount)++] = character;
			}

		} else if (character == ' ' || character == '2') {
			characters[(*charactersCount)++] = '\0';

		} else if (character == '7') {
			characters[(*charactersCount)++] = 31;

		} else if (character == '6') {
			characters[(*charactersCount)++] = 30;

		} else if (character == '5') {
			characters[(*charactersCount)++] = 29;

		} else if (character == '4') {
			characters[(*charactersCount)++] = 28;

		} else if (character == '3') {
			characters[(*charactersCount)++] = 27;

		} else if (character == '?' || character == '8') {
			characters[(*charactersCount)++] = '\x7F';
		}
	}
}

static int addKeyEventModifiersCharacters(KeyEvent keyEvent, char* buffer, int charactersCount) {
	int initialCharactersCount = charactersCount;

	bool controlKeyPressed = (KEY_EVENT_CONTROL_KEY_PRESSED_MASK & keyEvent) != 0;
	bool shiftKeyPressed = (KEY_EVENT_SHIFT_KEY_PRESSED_MASK & keyEvent) != 0;
	bool altKeyPressed = (KEY_EVENT_ALT_KEY_PRESSED_MASK & keyEvent) != 0;

	int modifiers = 0;

	if (shiftKeyPressed) {
		modifiers |= 0x1;

	} else if (altKeyPressed) {
		modifiers |= 0x2;

	} if (controlKeyPressed) {
		modifiers |= 0x4;
	}

	if (modifiers > 0) {
		if (initialCharactersCount == 0 || buffer[initialCharactersCount - 1] == '[') {
			buffer[charactersCount++] = '1';
		}
		buffer[charactersCount++] = ';';
		buffer[charactersCount++] = '0' + modifiers + 1;
		return charactersCount - initialCharactersCount;

	} else {
		return 0;
	}
}

static void keyEventSink(KeyEvent keyEvent) {
	struct TTY* tty = &ttys[foregroundTTYId];

	if ((KEY_EVENT_RELEASED_EVENT_MASK & keyEvent) == 0) {
		char character;
		bool isASCIICharacter = keyEventGetASCIICharacter(keyEvent, &character);
		bool controlKeyPressed = (KEY_EVENT_CONTROL_KEY_PRESSED_MASK & keyEvent) != 0;
		bool altKeyPressed = (KEY_EVENT_ALT_KEY_PRESSED_MASK & keyEvent) != 0;

		int characterCount = 0;
		char characters[16];

		if (isASCIICharacter) {
			if (controlKeyPressed && altKeyPressed) {
				characters[characterCount++] = '\x1B';
				handleKeyEventWithControlKeyPressed(tty, KEY_EVENT_SCAN_CODE_MASK & keyEvent, isASCIICharacter, character, characters, &characterCount);

			} else if (altKeyPressed) {
				characters[characterCount++] = '\x1B';
				character = transformCharAccordingToFlags(tty, character);
				if (character != '\0') {
					characters[characterCount++] = character;
				}

			} else if (controlKeyPressed) {
				handleKeyEventWithControlKeyPressed(tty, KEY_EVENT_SCAN_CODE_MASK & keyEvent, isASCIICharacter, character, characters, &characterCount);

			} else {
				character = transformCharAccordingToFlags(tty, character);
				if (character != '\0') {
					characters[characterCount++] = character;
				}
			}

		} else {
			switch (KEY_EVENT_SCAN_CODE_MASK & keyEvent) {
				case UP_ARROW_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = 'A';
					break;

				case DOWN_ARROW_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = 'B';
					break;

				case LEFT_ARROW_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = 'D';
					break;

				case RIGHT_ARROW_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = 'C';
					break;

				case ESCAPE_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					break;

				case BACKSPACE_KEY_SCAN_CODE:
					if (altKeyPressed) {
						characters[characterCount++] = '\x1B';
					}
					if (controlKeyPressed) {
						characters[characterCount++] = 8;
					} else {
						characters[characterCount++] = '\x7F';
					}
					break;

				case F1_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'M';
					break;

				case F2_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'N';
					break;

				case F3_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'O';
					break;

				case F4_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'P';
					break;

				case F5_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'Q';
					break;

				case F6_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'R';
					break;

				case F7_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'S';
					break;

				case F8_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'T';
					break;

				case F9_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'U';
					break;

				case F10_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'V';
					break;

				case F11_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'W';
					break;

				case F12_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = 'X';
					break;

				case PAGE_UP_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = '5';
					characters[characterCount++] = '~';
					break;

				case PAGE_DOWN_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = '6';
					characters[characterCount++] = '~';
					break;

				case HOME_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = 'H';
					break;

				case END_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = 'F';
					break;

				case INSERT_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = '2';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = '~';
					break;

				case DELETE_KEY_SCAN_CODE:
					characters[characterCount++] = '\x1B';
					characters[characterCount++] = '[';
					characters[characterCount++] = '3';
					characterCount += addKeyEventModifiersCharacters(keyEvent, characters, characterCount);
					characters[characterCount++] = '~';
					break;
			}
		}

		if (characterCount > 0) {
			bool echo;
			if (!handleSpecialInputSequences(tty, characters, characterCount, &echo)) {
				bool canonicalMode = (tty->termiosInstance.c_lflag & ICANON) != 0;

				if (canonicalMode && ((KEY_EVENT_SCAN_CODE_MASK & keyEvent) == BACKSPACE_KEY_SCAN_CODE || (characterCount == 1 && tty->termiosInstance.c_cc[VERASE] == characters[0]))) {
					echo = doCanonicalBackspace(tty);

				} else if (canonicalMode && (tty->termiosInstance.c_lflag & IEXTEN) != 0 && characterCount == 1 && tty->termiosInstance.c_cc[VWERASE] == characters[0]) {
					echo = doCanonicalEraseWord(tty);

				} else {
					echo = true;
					writeToTTYInput(tty, characters, characterCount, characterCount == 1 && characters[0] == '\n');
				}
			}

			if (echo && !doEcho(tty, keyEvent, isASCIICharacter, characters, characterCount)) {
				resetScroll(tty);
			}
		}
	}
}

static bool doScrollUp(int delta) {
	struct TTY* tty = &ttys[foregroundTTYId];

	if (tty->currentOutputRingBuffer == &tty->mainOutputRingBuffer) {
		int columnCount = vgaGetColumnCount();
		int rowCount = vgaGetRowCount();

		uint32_t frameBufferSize = columnCount * rowCount * sizeof(uint16_t);
		uint32_t rowSize = columnCount * sizeof(uint16_t);

		int size = ringBufferSize(tty->currentOutputRingBuffer) - (*tty->currentNextCharacterColumn * sizeof(uint16_t));

		if (size > frameBufferSize) {
			int adjustedSize = size - (size % rowSize);
			int maxScrollDelta = (adjustedSize - frameBufferSize) / rowSize - *tty->currentScrollDelta + 1;

			if (maxScrollDelta >= delta) {
				*tty->currentScrollDelta += delta;
				copyViewport(tty);
			} else if (maxScrollDelta > 0) {
				*tty->currentScrollDelta += maxScrollDelta;
				copyViewport(tty);
			}
		}

		return true;

	} else {
		return false;
	}
}

static bool doScrollDown(int delta) {
	struct TTY* tty = &ttys[foregroundTTYId];

	if (tty->currentOutputRingBuffer == &tty->mainOutputRingBuffer) {
		if (*tty->currentScrollDelta > delta) {
			*tty->currentScrollDelta -= delta;
		} else {
			*tty->currentScrollDelta = 0;
		}
		copyViewport(tty);

		return true;

	} else {
		return false;
	}
}

bool ttyIsValidTTYId(int ttyId) {
	return 0 <= ttyId && ttyId < TTY_COUNT;
}

bool ttySetForegroundTTY(int ttyId) {
	assert(ttyIsValidTTYId(ttyId));

	if (ttyId != foregroundTTYId) {
		struct TTY* tty = &ttys[ttyId];
		copyViewport(tty);
		foregroundTTYId = ttyId;
	}

	return true;
}

struct TTYStreamWriter {
	struct StreamWriter streamWriter;
	int ttyId;
};

static ssize_t ttyStreamWriterWrite(struct TTYStreamWriter* ttyStreamWriter, const char* buffer, size_t bufferSize, int* errorId) {
	*errorId = 0;
	for (int i = 0; i < bufferSize; i++) {
		writeToOutput(ttyStreamWriter->ttyId, buffer[i], false);
	}
	return bufferSize;
}

static void ttyStreamWriterInitialize(struct TTYStreamWriter* ttyStreamWriter, int ttyId) {
	streamWriterInitialize(&ttyStreamWriter->streamWriter, (ssize_t (*)(struct StreamWriter*, const void*, size_t, int*)) &ttyStreamWriterWrite);
	ttyStreamWriter->ttyId = ttyId;
}

void ttyWriteToOutputVaFormat(int ttyId, const char *format, va_list ap) {
	struct TTYStreamWriter ttyStreamWriter;
	ttyStreamWriterInitialize(&ttyStreamWriter, ttyId);
	streamWriterVaFormat(&ttyStreamWriter.streamWriter, format, ap);
}

void ttyWriteToOutputFormat(int ttyId, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	ttyWriteToOutputVaFormat(ttyId, format, ap);
	va_end(ap);
}

void ttyInitialize(struct multiboot_info *multiboot_info) {
	memset(&virtualFileSystemOperations, 0, sizeof(struct VirtualFileSystemOperations));
	virtualFileSystemOperations.open = &open;
	virtualFileSystemOperations.write = &write;
	virtualFileSystemOperations.read = &read;
	virtualFileSystemOperations.status = &status;
	virtualFileSystemOperations.getMode = &getMode;
	virtualFileSystemOperations.manipulateDeviceParameters = &manipulateDeviceParameters;
	virtualFileSystemOperations.startIoEventMonitoring = &startIoEventMonitoring;
	virtualFileSystemOperations.stopIoEventMonitoring = &stopIoEventMonitoring;

	for (int i = 0; i < TTY_COUNT; i++) {
		struct TTY* tty = &ttys[i];
		tty->id = i;

		tty->currentSavedNextCharacterColumn = &tty->mainSavedNextCharacterColumn;
		tty->currentSavedNextCharacterRow = &tty->mainSavedNextCharacterRow;

		tty->currentNextCharacterColumn = &tty->mainNextCharacterColumn;
		tty->currentNextCharacterRow = &tty->mainNextCharacterRow;
		tty->currentCursorArtificiallyOnEdgeDueToLastWrite = &tty->mainCursorArtificiallyOnEdgeDueToLastWrite;

		tty->currentOutputRingBuffer = &tty->mainOutputRingBuffer;
		tty->currentScrollDelta = &tty->mainScrollDelta;
		tty->currentInvertColors = &tty->mainInvertColors;

		tty->currentForegroundColor = &tty->mainForegroundColor;
		tty->currentBackgroundColor = &tty->mainBackgroundColor;

		ringBufferInitialize(&tty->inputRingBuffer, &tty->inputBuffer, TTY_INPUT_BUFFER_CAPACITY);
		ringBufferInitialize(&tty->mainOutputRingBuffer, &tty->mainOutputBuffer, TTY_MAIN_OUTPUT_BUFFER_CAPACITY);

		*tty->currentForegroundColor = DEFAULT_RENDITION_FOREGROUND_COLOR;
		*tty->currentBackgroundColor = DEFAULT_RENDITION_BACKGROUND_COLOR;

		memset(&ttysVirtualFileSystemNodes[i], 0, sizeof(struct TTYVirtualFileSystemNode));
		ttysVirtualFileSystemNodes[i].tty = tty;
		ttysVirtualFileSystemNodes[i].virtualFileSystemNode.operations = &virtualFileSystemOperations;
		ttysVirtualFileSystemNodes[i].mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

		struct termios* termiosInstance = &tty->termiosInstance;
		memset(termiosInstance, 0, sizeof(struct termios));
		termiosInstance->c_lflag = ECHO | ECHOE | ECHOK | ECHOCTL | ICANON | ISIG | IEXTEN;
		termiosInstance->c_iflag = ICRNL;
		termiosInstance->c_cflag = CS8 | CREAD;
		termiosInstance->c_oflag = OPOST;
		termiosInstance->c_cc[VSUSP] = '\032'; /* ^Z */
		termiosInstance->c_cc[VQUIT] = '\034'; /* ^\ */
		termiosInstance->c_cc[VINTR] = '\003'; /* ^C */
		termiosInstance->c_cc[VKILL] = '\025'; /* ^U */
		termiosInstance->c_cc[VEOF] = '\004'; /* ^D */
		termiosInstance->c_cc[VEOL] = '\0'; /* \0 */
		termiosInstance->c_cc[VERASE] = -1; /* ^? */
		termiosInstance->c_cc[VWERASE] = '\027'; /* ^W */

		tty->isCursorEnabled = true;

		doubleLinkedListInitialize(&tty->ioEventMonitoringContextList);
	}

	int stringStreamWriterBufferSize = 1024;
	char stringStreamWriterBuffer[stringStreamWriterBufferSize];
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, stringStreamWriterBuffer, stringStreamWriterBufferSize);
	vgaInitialize(multiboot_info, &stringStreamWriter);
	streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '\0');

	int columnCount = vgaGetColumnCount();
	int rowCount = vgaGetRowCount();

	/* Clear the screen. */
	{
		uint16_t characterAndColor = (COMBINE_COLORS(DEFAULT_RENDITION_FOREGROUND_COLOR, DEFAULT_RENDITION_BACKGROUND_COLOR) << 8) | ' ';
		uint16_t* frameBuffer = vgaGetFrameBuffer();
		for (int row = 0; row < rowCount; row++) {
			for (int column = 0; column < columnCount; column++) {
				frameBuffer[row * columnCount + column] = characterAndColor;
			}
		}
	}

	/* We can write only after VGA initialization. */
	for (int i = 0; i < TTY_COUNT; i++) {
		struct TTY* tty = &ttys[i];

		uint32_t rowSize = columnCount * sizeof(uint16_t);

		uint16_t characterAndColor = combineCharacterAndColor(tty, ' ');

		assert(TTY_ALTERNATIVE_OUTPUT_BUFFER_CAPACITY >= columnCount * rowCount * sizeof(uint16_t));
		ringBufferInitialize(&tty->alternativeOutputRingBuffer, &tty->alternativeOutputBuffer, columnCount * rowCount * sizeof(uint16_t));

		for (int row = 0; row < rowCount; row++) {
			for (int column = 0; column < columnCount; column++) {
				ringBufferWrite(&tty->mainOutputRingBuffer, &characterAndColor, sizeof(uint16_t));
				ringBufferWrite(&tty->alternativeOutputRingBuffer, &characterAndColor, sizeof(uint16_t));
			}
		}
		assert(tty->mainNextCharacterColumn == 0);
		assert(tty->mainNextCharacterRow == 0);

		int newCapacity = tty->mainOutputRingBuffer.capacity - (tty->mainOutputRingBuffer.capacity % rowSize);
		assert(newCapacity >= ringBufferSize(&tty->mainOutputRingBuffer));
		assert(newCapacity % rowSize == 0);
		tty->mainOutputRingBuffer.capacity = newCapacity;

		ttyWriteToOutputFormat(i, "\x1B[0;97m"
			"    __  ___        ____  _____\n"
			"   /  |/  /__  __ / __ \\/ ___/\n"
			"  / /|_/ // / / // / / /\\__ \\\n"
			" / /  / // /_/ // /_/ /___/ / \n"
			"/_/  /_/ \\__, / \\____//____/\n"
			"        /____/\n"
			"\x1B[m\n");
		ttyWriteToOutputFormat(i, "Welcome to \x1B[0;97mMyOS\x1B[m!\nTTY #%d\n\n", i + 1);
	}

	logDebug("VGA initialization details:\n%s", stringStreamWriterBuffer);

	bool result;
	for (int i = 0; i < TTY_COUNT; i++) {
		assert(i < NUMBER_OF_F_KEYS);
		result = keyboardRegisterKeyStroke(KEYBOARD_F_KEYS_SCAN_CODES[i], KEY_EVENT_CONTROL_KEY_PRESSED_MASK,
			false, (bool(*)(void*))&ttySetForegroundTTY, (void*)i);
		assert(result);
	}

	result = keyboardRegisterKeyStroke(PAGE_UP_KEY_SCAN_CODE, KEY_EVENT_SHIFT_KEY_PRESSED_MASK,
		false, (bool(*)(void*))&doScrollUp, (void*)COARSE_SCROLL_DELTA);
	assert(result);
	result = keyboardRegisterKeyStroke(PAGE_DOWN_KEY_SCAN_CODE, KEY_EVENT_SHIFT_KEY_PRESSED_MASK,
		false, (bool(*)(void*))&doScrollDown, (void*)COARSE_SCROLL_DELTA);
	assert(result);

	result = keyboardRegisterKeyStroke(UP_ARROW_KEY_SCAN_CODE, KEY_EVENT_SHIFT_KEY_PRESSED_MASK,
		false, (bool(*)(void*))&doScrollUp, (void*)FINE_SCROLL_DELTA);
	assert(result);
	result = keyboardRegisterKeyStroke(DOWN_ARROW_KEY_SCAN_CODE, KEY_EVENT_SHIFT_KEY_PRESSED_MASK,
		false, (bool(*)(void*))&doScrollDown, (void*)FINE_SCROLL_DELTA);
	assert(result);

	result = keyboardRegisterKeyStroke(HOME_KEY_SCAN_CODE, KEY_EVENT_SHIFT_KEY_PRESSED_MASK,
		false, (bool(*)(void*))&doScrollUp, (void*)LONG_MAX);
	assert(result);
	result = keyboardRegisterKeyStroke(END_KEY_SCAN_CODE, KEY_EVENT_SHIFT_KEY_PRESSED_MASK,
		false, (bool(*)(void*))&doScrollDown, (void*)LONG_MAX);
	assert(result);

	keyboardSetKeyEventSink(&keyEventSink);
}

void ttyRegisterDevices(void) {
	int bufferSize = 16;
	char buffer[bufferSize];

	for (int i = 0; i < TTY_COUNT; i++) {
		assert(TTY_COUNT <= 12);
		sprintf(buffer, "tty%.2d", i + 1);
		devicesFileSystemRegisterDevice(&ttysVirtualFileSystemNodes[i].virtualFileSystemNode, buffer);
	}
}

void ttyHandleProcessGroupBecameEmpty(struct ProcessGroup* processGroup) {
	for (int i = 0; i < TTY_COUNT; i++) {
		struct TTY* tty = &ttys[i];
		if (tty->foregroundProcessGroup == processGroup) {
			assert(processGroup->session == tty->sessionBeingControlled);
			processGroupManagerReleaseReservation(tty->foregroundProcessGroup);
			tty->foregroundProcessGroup = NULL;
			break;
		}
	}
}

void ttyHandleSessionLeaderTermination(int ttyId, struct Process* currentProcess) {
	assert(ttyIsValidTTYId(ttyId));

	struct TTY* tty = &ttys[ttyId];
	assert(tty->sessionBeingControlled != NULL);
	tty->sessionBeingControlled->controllingTTYId = -1;
	tty->sessionBeingControlled = NULL;
	if (tty->foregroundProcessGroup != NULL) {
		/*
		 * If the process is a controlling process, the SIGHUP signal shall be sent to each process
       * in the foreground process group of the controlling terminal belonging to the calling process.
       *
       * https://pubs.opengroup.org/onlinepubs/9699919799/functions/_exit.html
		 */
		signalServicesGenerateSignal(currentProcess, -tty->foregroundProcessGroup->id, SIGHUP, false, NULL);

		/*
       * If the process is a controlling process, the controlling terminal associated with the session
       * shall be disassociated from the session, allowing it to be acquired by a new controlling process.
       *
       * https://pubs.opengroup.org/onlinepubs/9699919799/functions/_exit.html
       */
		processGroupManagerReleaseReservation(tty->foregroundProcessGroup);
		tty->foregroundProcessGroup = NULL;
	}
}

APIStatusCode ttyPrintDebugReport(void) {
	int bufferSize = 1024;
	char buffer[bufferSize];
	struct StringStreamWriter stringStreamWriter;

	logDebug("TTY manager report:\n");

	for (int i = 0; i < TTY_COUNT; i++) {
		struct TTY* tty = &ttys[i];
		struct TTYVirtualFileSystemNode* ttyVirtualFileSystemNode = &ttysVirtualFileSystemNodes[i];

		stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);

		streamWriterFormat(&stringStreamWriter.streamWriter, "tty%.2d\n", tty->id + 1);

		streamWriterFormat(&stringStreamWriter.streamWriter, "  foregroundProcessGroup=%d\n", tty->foregroundProcessGroup ? tty->foregroundProcessGroup->id : -1);
		streamWriterFormat(&stringStreamWriter.streamWriter, "  ioEventMonitoringContextList=%d\n", doubleLinkedListSize(&tty->ioEventMonitoringContextList));
		streamWriterFormat(&stringStreamWriter.streamWriter, "  inputRingBuffer=%d\n", ringBufferSize(&tty->inputRingBuffer));
		streamWriterFormat(&stringStreamWriter.streamWriter, "  waitingIOProcessList=%d\n", doubleLinkedListSize(&ttyVirtualFileSystemNode->virtualFileSystemNode.waitingIOProcessList));

		stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
		logDebug("%s", buffer);
	}

	return SUCCESS;
}
