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
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util/date_time_utils.h"
#include "util/formatter.h"
#include "util/string_stream_writer.h"

static ssize_t callFormatAndGetRequiredLength(struct StringStreamWriter* stringStreamWriter, size_t* requiredLength, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	ssize_t result = formatterFormat(&stringStreamWriter->streamWriter, format, ap, requiredLength);
	va_end(ap);
	return result;
}

static ssize_t callFormat(struct StringStreamWriter* stringStreamWriter, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	ssize_t result = formatterFormat(&stringStreamWriter->streamWriter, format, ap, NULL);
	va_end(ap);
	return result;
}

static void testFormat1(void) {
	#define BUFFER_SIZE 2
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	ssize_t result = callFormat(&stringStreamWriter, "ABCDEF");
	assert(result == 2);
}

static void testFormat2(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	callFormat(&stringStreamWriter, "%d %X \"%s\" %u %d", -123, 0xABCD1234, "testing", 0xFFFFFFFF, INT_MIN);
	assert(strcmp("-123 ABCD1234 \"testing\" 4294967295 -2147483648", buffer) == 0);
}

static void testFormat3(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	struct StringStreamWriter stringStreamWriter;
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);

	callFormat(&stringStreamWriter, "%.3s", "ABCDEFG");
	assert(strcmp("ABC", buffer) == 0);

	callFormat(&stringStreamWriter, "%.1s", "DDDDDDDDDDDDDDDDDD");
	assert(strcmp("ABCD", buffer) == 0);

	callFormat(&stringStreamWriter, "%.s", "EEEE");
	assert(strcmp("ABCD", buffer) == 0);
}

static void testFormat4(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 7
	char buffer[BUFFER_SIZE];
	ssize_t result;
	size_t requiredLength;
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%b|%b|%b", true, false, false);
	assert(result == BUFFER_SIZE);
	assert(requiredLength == 4 + 1 + 5 + 1 + 5);
	assert(strncmp("true|fa", buffer, BUFFER_SIZE) == 0);
}

static void testFormat5(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "%llX %llX", 0xFEDCBA9876543210LL, 0x123456789ABCDEFLL);
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("FEDCBA9876543210 123456789ABCDEF", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%20llX| |%20llX|", 0xFEDCBA9876543210LL, 0x123456789ABCDEFLL);
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|    FEDCBA9876543210| |     123456789ABCDEF|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%0*llX| |%018llX|", 18, 0xFEDCBA9876543210LL, 0x123456789ABCDEFLL);
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|00FEDCBA9876543210| |000123456789ABCDEF|", buffer, BUFFER_SIZE) == 0);
}

static void testFormat6(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	ssize_t result;
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%10.5d|", 123);
	assert(strncmp("|     00123|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%10d|", 123);
	assert(strncmp("|       123|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%d|", 123);
	assert(strncmp("|123|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%010d|", 111123);
	assert(strncmp("|0000111123|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%.*d|", 50, 111123);
	assert(strncmp("|00000000000000000000000000000000000000000000111123|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%*d|", 45, 111123);
	assert(strncmp("|                                       111123|", buffer, result) == 0);
}

static void testFormat7(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%10s|", "ABC");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|       ABC|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%.10s|", "ABC");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|ABC|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%10.4s|", "ABCDEFGH");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|      ABCD|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%*.*s|", 10, 4, "ABCDEFGH");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|      ABCD|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%.**s|", 4, 10, "ABCDEFGH");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|      ABCD|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%.1000s|", "ABCDEFGH");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|ABCDEFGH|", buffer, BUFFER_SIZE) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	callFormat(&stringStreamWriter, "|%*s|", 10, "ABCDEFGH");
	stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
	assert(strncmp("|  ABCDEFGH|", buffer, BUFFER_SIZE) == 0);
}

static void testFormat8(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	ssize_t result;
	struct StringStreamWriter stringStreamWriter;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%05d|", -5);
	assert(strncmp("|-0005|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%5d|", -105);
	assert(strncmp("| -105|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%10.5d|", -123);
	assert(strncmp("|    -00123|", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormat(&stringStreamWriter, "|%2.5d|", -78);
	assert(strncmp("|-0078|", buffer, result) == 0);
}

static void testFormat9(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 6
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	ssize_t result;
	size_t requiredLength;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "|%d|%d|", -5, -1111);
	assert(result == BUFFER_SIZE);
	assert(requiredLength == 10);
	assert(strncmp("|-5|-11", buffer, BUFFER_SIZE) == 0);
}

static void testFormat10(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	ssize_t result;
	size_t requiredLength;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "\xFF\xFE\xFD\xFC");
	assert(result == 4);
	assert(requiredLength == 4);
	assert(strncmp("\xFF\xFE\xFD\xFC", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%s", "||||\xFF\xFE\xFD\xFC\xFB");
	assert(result == 9);
	assert(requiredLength == 9);
	assert(strncmp("||||\xFF\xFE\xFD\xFC\xFB", buffer, result) == 0);
}

static void testFormat11(void) {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	ssize_t result;
	size_t requiredLength;

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%Lf", 123.456L);
	assert(result == 3 + 1 + 6);
	assert(requiredLength == 3 + 1 + 6);
	assert(strncmp("123.456000", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%Lf", -0.L);
	assert(result ==  1 + 1 + 1 + 6);
	assert(requiredLength == 1 + 1 + 1 + 6);
	assert(strncmp("-0.000000", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%Lf", -10.L);
	assert(result ==  1 + 2 + 1 + 6);
	assert(requiredLength == 1 + 2 + 1 + 6);
	assert(strncmp("-10.000000", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%Lf", -(1.L/0.L));
	assert(result ==  4);
	assert(requiredLength == 4);
	assert(strncmp("-inf", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%5F", -NAN);
	assert(result ==  5);
	assert(requiredLength == 5);
	assert(strncmp(" -NAN", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%8.2F", 123.);
	assert(result ==  2 + 3 + 1 + 2);
	assert(requiredLength == 2 + 3 + 1 + 2);
	assert(strncmp("  123.00", buffer, result) == 0);

	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = callFormatAndGetRequiredLength(&stringStreamWriter, &requiredLength, "%08.2F", 567.12);
	assert(result ==  2 + 3 + 1 + 2);
	assert(requiredLength == 2 + 3 + 1 + 2);
	assert(strncmp("00567.12", buffer, result) == 0);
}

static void testFormatDateTime1() {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	ssize_t result;
	size_t requiredLength;
	const char* expected;
	struct tm tmInstance;

	/* GMT: Saturday, 30 May 2020 23:59:55 */
	dateTimeUtilsUnixTimeToTmInstance(1590883195, &tmInstance);
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%a %A %b %B %% %n %t %Y %d %z %y %p %H:%M:%S %I %e %m", &tmInstance, true, &requiredLength);
	expected = "Sat Saturday May May % \n \t 2020 30 UTC 20 PM 23:59:55 11 30 05";
	assert(strcmp(expected, buffer) == 0);
	assert(requiredLength == result);

	/* GMT: Thursday, 24 October 1929 08:25:38 */
	dateTimeUtilsUnixTimeToTmInstance(-1268235262, &tmInstance);
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%c", &tmInstance, true, &requiredLength);
	expected = "Thu Oct 24 08:25:38 1929";
	assert(strcmp(expected, buffer) == 0);
	assert(requiredLength == result);

	/* GMT: Tuesday, 27 July 1982 10:58:20 */
	dateTimeUtilsUnixTimeToTmInstance(396615500, &tmInstance);
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%D %F %T", &tmInstance, true, &requiredLength);
	expected = "07/27/82 1982-07-27 10:58:20";
	assert(strcmp(expected, buffer) == 0);
	assert(requiredLength == result);

	/* GMT: Tuesday, 19 January 2038 03:14:07 */
	dateTimeUtilsUnixTimeToTmInstance(INT_MAX, &tmInstance);
	stringStreamWriterInitialize(&stringStreamWriter, buffer, BUFFER_SIZE);
	result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%r |%R|%l%k", &tmInstance, true, &requiredLength);
	expected = "03:14:07 AM |03:14| 3 3";
	assert(strcmp(expected, buffer) == 0);
	assert(requiredLength == result);
}

static void testFormatDateTime2() {
	#undef BUFFER_SIZE
	#define BUFFER_SIZE 128
	char buffer[BUFFER_SIZE];
	struct StringStreamWriter stringStreamWriter;
	ssize_t result;
	size_t requiredLength;
	const char* expected;
	struct tm tmInstance;

	/* GMT: Tuesday, 19 January 2038 03:14:07 */
	memset(buffer, '*', BUFFER_SIZE);
	buffer[BUFFER_SIZE - 1] = '\0';

	dateTimeUtilsUnixTimeToTmInstance(INT_MAX, &tmInstance);
	stringStreamWriterInitialize(&stringStreamWriter, buffer, 0);
	result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%c", &tmInstance, true, &requiredLength);
	expected = "Tue Jan 19 03:14:07 2038";
	assert(result < 0);
	assert(requiredLength == strlen(expected) + 1);
	assert(strspn(buffer, "*") == BUFFER_SIZE - 1);

	/* GMT: Sunday, 14 September 2003 23:44:10 */
	memset(buffer, '+', BUFFER_SIZE);
	buffer[BUFFER_SIZE - 1] = '\0';

	dateTimeUtilsUnixTimeToTmInstance(1063583050, &tmInstance);
	stringStreamWriterInitialize(&stringStreamWriter, buffer, 12);
	result = formatterFormatDateTime(&stringStreamWriter.streamWriter, "%F %T", &tmInstance, true, &requiredLength);
	expected = "2003/09/14 23:44:10";
	assert(result == 12);
	assert(requiredLength == strlen(expected) + 1);
	assert(strncmp(expected, buffer, 12));
	assert(strspn(buffer + 12, "+") == BUFFER_SIZE - 1 - 12);
}

int main(int argc, char** argv) {
	testFormat1();
	testFormat2();
	testFormat3();
	testFormat4();
	testFormat5();
	testFormat6();
	testFormat7();
	testFormat8();
	testFormat9();
	testFormat10();
	testFormat11();

	testFormatDateTime1();
	testFormatDateTime2();

	return 0;
}
