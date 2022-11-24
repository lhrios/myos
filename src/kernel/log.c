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

#include <stdbool.h>
#include <string.h>

#include "kernel/error_handler.h"
#include "kernel/log.h"
#include "kernel/tty.h"

#include "util/formatter.h"

#define LOG_TTY_ID 0

static enum LogLevel localLogLevel = WARN_LOG_LEVEL;

static const char * const logLevelNames[] = {
	NULL,
	"DEBUG",
	"INFO",
	"WARN",
	"FATAL"
};

void doAssert(int condition, const char* format, ...) {
	if (!condition) {
		va_list ap;
		va_start(ap, format);
		ttyWriteToOutputVaFormat(LOG_TTY_ID, "\x1B[41;93m", ap);
		ttyWriteToOutputVaFormat(LOG_TTY_ID, format, ap);
		ttyWriteToOutputVaFormat(LOG_TTY_ID, "\x1B[m", ap);
		errorHandlerStopExecutionDueToFatalError();
		va_end(ap);
	}
}

void logBringLogTTYToForeground(void) {
	ttySetForegroundTTY(LOG_TTY_ID);
}

void logFormat(enum LogLevel logLevel, const char* format, ...) {
	if (logLevel >= localLogLevel) {
		va_list ap;

		va_start(ap, format);
		ttyWriteToOutputVaFormat(LOG_TTY_ID, format, ap);
		va_end(ap);
	}
}

enum LogLevel logLogLevelByName(const char* logLevelName) {
	for (int i = 1; i < sizeof(logLevelNames) / sizeof(char*); i++) {
		if (strcasecmp(logLevelName, logLevelNames[i]) == 0) {
			return i;
		}
	}

	return UNKNOWN_LOG_LEVEL;
}

void logSetLogLevel(enum LogLevel logLevel) {
	if (logLevel != UNKNOWN_LOG_LEVEL) {
		localLogLevel = logLevel;
	}
}
