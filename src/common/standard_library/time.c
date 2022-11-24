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

#include <assert.h>
#include <string.h>
#include <time.h>

#include "util/date_time_utils.h"
#include "util/formatter.h"
#include "util/string_stream_writer.h"

char* tzname[2] = {
	"UTC",
	"UTC"
};

struct tm* localtime(const time_t* timeInstance) {
	// TODO: Implement me!
	return gmtime(timeInstance);
}

static struct tm tmInstance;
struct tm* gmtime(const time_t* timeInstance) {
	dateTimeUtilsUnixTimeToTmInstance(*timeInstance, &tmInstance);
	return &tmInstance;
}

void tzset(void) {
}

size_t strftime(char* buffer, size_t bufferSize, const char* format, const struct tm* tmInstance) {
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, bufferSize);

	ssize_t result = formatterFormatDateTime(&stringStreamWriter.streamWriter, format, tmInstance, true, NULL);
	if (result <= 0) {
		return 0;
	} else {
		return result - 1;
	}
}

#define ASCTIME_BUFFER_SIZE 64
static char asctimeBuffer[ASCTIME_BUFFER_SIZE];
char* asctime(const struct tm* tmInstance) {
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, asctimeBuffer, ASCTIME_BUFFER_SIZE);

	size_t requiredLength;
	ssize_t result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%c", tmInstance, true, &requiredLength);
	assert(result > 0);
	assert(requiredLength <= ASCTIME_BUFFER_SIZE);

	return asctimeBuffer;
}

char* ctime(const time_t* timeInstance) {
	return asctime(gmtime(timeInstance));
}
