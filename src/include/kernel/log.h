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

#ifndef KERNEL_LOG_H
	#define KERNEL_LOG_H

	#include <stdarg.h>

	enum LogLevel {
		UNKNOWN_LOG_LEVEL = 0,
		DEBUG_LOG_LEVEL = 1,
		INFO_LOG_LEVEL = 2,
		WARN_LOG_LEVEL = 3,
		FATAL_LOG_LEVEL = 4
	};

	#define logDebug(FORMAT, ...) \
		logFormat(DEBUG_LOG_LEVEL, "[%s %s %d] " FORMAT "\n", "\x1B[92mDBG\x1B[m", __FILE__, __LINE__, ##__VA_ARGS__)

	#define logInfo(FORMAT, ...) \
		logFormat(INFO_LOG_LEVEL, "[%s %s %d] " FORMAT "\n", "\x1B[94mINF\x1B[m", __FILE__, __LINE__, ##__VA_ARGS__)

	#define logWarn(FORMAT, ...) \
		logFormat(WARN_LOG_LEVEL, "[%s %s %d] " FORMAT "\n", "\x1B[93mWRN\x1B[m", __FILE__, __LINE__, ##__VA_ARGS__)

	#define logFatal(FORMAT, ...) \
		logFormat(FATAL_LOG_LEVEL, "[%s %s %d] " FORMAT "\n", "\x1B[91mFTL\x1B[m", __FILE__, __LINE__, ##__VA_ARGS__)

	void logFormat(enum LogLevel logLevel, const char* format, ...);
	void logBringLogTTYToForeground(void);
	void logSetLogLevel(enum LogLevel logLevel);
	enum LogLevel logLogLevelByName(const char* logLevelName);

	// TODO: Use a streamWriter to enable the generation of large outputs

#endif
