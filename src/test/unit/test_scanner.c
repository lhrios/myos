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
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "user/util/scanner.h"

#include "util/math_utils.h"
#include "util/string_stream_reader.h"

#define DELTA 0.00001

static void testScannerParseInt32(const char* string, int base, bool allowSpaceBefore, bool allowSign, const char* expectedStartOfNotProcessedString, int32_t expectedOutput, int expectedErrorId) {
	const char* startOfNotProcessedString;
	int32_t output;

	int returnedErrorId = scannerParseInt32(string, base, allowSpaceBefore, allowSign, expectedStartOfNotProcessedString != NULL ? &startOfNotProcessedString : NULL, &output);
	if (returnedErrorId != expectedErrorId) {
		printf("returnedErrorId=%d expectedErrorId=%d\n", returnedErrorId, expectedErrorId);
		assert(false);
	}
	if (output != expectedOutput) {
		printf("output=%d expectedOutput=%d\n", output, expectedOutput);
		assert(false);
	}
	assert(output == expectedOutput);
	if (expectedStartOfNotProcessedString != NULL) {
		if(strcmp(startOfNotProcessedString, expectedStartOfNotProcessedString) != 0) {
			printf("startOfNotProcessedString='%s' expectedStartOfNotProcessedString='%s'\n", startOfNotProcessedString, expectedStartOfNotProcessedString);
			assert(false);
		}
	}
}

static void testScannerParseUint32(const char* string, int base, bool allowSpaceBefore, const char* expectedStartOfNotProcessedString, uint32_t expectedOutput, int expectedErrorId) {
	const char* startOfNotProcessedString;
	uint32_t output;

	int returnedErrorId = scannerParseUint32(string, base, allowSpaceBefore, expectedStartOfNotProcessedString != NULL ? &startOfNotProcessedString : NULL, &output);
	if (returnedErrorId != expectedErrorId) {
		printf("returnedErrorId=%d expectedErrorId=%d\n", returnedErrorId, expectedErrorId);
		assert(false);
	}
	if (output != expectedOutput) {
		printf("output=%u expectedOutput=%u\n", output, expectedOutput);
		assert(false);
	}
	assert(output == expectedOutput);
	if (expectedStartOfNotProcessedString != NULL) {
		if(strcmp(startOfNotProcessedString, expectedStartOfNotProcessedString) != 0) {
			printf("startOfNotProcessedString=%s expectedStartOfNotProcessedString=%s\n", startOfNotProcessedString, expectedStartOfNotProcessedString);
			assert(false);
		}
	}
}

static void test1(void) {
	testScannerParseInt32("123", 10, false, false, NULL, 123, 0);
	testScannerParseInt32("123", 10, false, false, "", 123, 0);
	testScannerParseInt32(" 123", 10, false, false, " 123", 0, 0);
	testScannerParseInt32(" 123", 10, true, false, "", 123, 0);
	testScannerParseInt32(" +123", 10, true, false, " +123", 0, 0);
	testScannerParseInt32(" +123", 10, true, true, "", 123, 0);
	testScannerParseInt32(" +123 45", 10, true, true, " 45", 123, 0);
	testScannerParseInt32(" +0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000123 45", 10, true, true, " 45", 123, 0);
	testScannerParseInt32("	12344444444444444444444444444444444444444444444444", 10, false, false, "	12344444444444444444444444444444444444444444444444", 0, 0);
}

static void test2(void) {
	testScannerParseInt32("12344444444444444444444444444444444444444444444444", 10, false, false, "", (int32_t) INT_MAX, ERANGE);
	testScannerParseInt32("	-12344444444444444444444444444444444444444444444444", 10, true, true, "", (int32_t) INT_MIN, ERANGE);
	testScannerParseInt32(" +2147483647", 10, true, true, "", (int32_t) INT_MAX, 0);
	testScannerParseInt32("-2147483648", 10, false, true, "", (int32_t) INT_MIN, 0);
}

static void test3(void) {
	testScannerParseInt32("100", 2, false, true, "", 4, 0);
	testScannerParseInt32("89CD52", 16, false, true, "", 9030994, 0);
	testScannerParseInt32("42346522", 8, false, true, "", 9030994, 0);
	testScannerParseInt32("-42346522", 8, false, true, "", -9030994, 0);
	testScannerParseInt32("   -0xFED", 0, true, true, "", -4077, 0);
	testScannerParseInt32("   -07777", 0, true, true, "", -4095, 0);
	testScannerParseInt32("  \t\r\n  1237777", 10, true, true, "", 1237777, 0);
}

static void test4(void) {
	testScannerParseInt32("   -0xF ED", 0, true, true, " ED", -15, 0);
	testScannerParseInt32("   -0xF ED", 0, false, true, "   -0xF ED", 0, 0);
	testScannerParseInt32("   -0xF ED", 0, true, false, "   -0xF ED", 0, 0);
	testScannerParseInt32("   -01F ED", 0, true, true, "F ED", -1, 0);
	testScannerParseInt32("   +0179FED", 0, true, true, "9FED", 15, 0);
	testScannerParseInt32("9875   ", 10, false, false, "   ", 9875, 0);
	testScannerParseInt32("ZZZ|", 36, false, false, "|", 46655, 0);
}

static void test5(void) {
	testScannerParseUint32("   12345", 0, true, "", 12345, 0);
	testScannerParseUint32("   4294967295EEEE", 0, true, "EEEE", UINT_MAX, 0);
	testScannerParseUint32("4294967295 EEEE", 0, false, " EEEE", UINT_MAX, 0);
	testScannerParseUint32("-42", 0, false, "-42", 0, 0);
	testScannerParseUint32("0xFFFFFFFF", 0, false, "", UINT_MAX, 0);
	testScannerParseUint32("0XFFFFFFFF", 0, false, "", UINT_MAX, 0);
	testScannerParseUint32(" 0XFFFFFFFF", 16, true, "", UINT_MAX, 0);
	testScannerParseUint32("   ZZZZZ|", 36, true, "|", 60466175, 0);
}

static void memoryAllocatorRelease(void* context, void* pointer) {
	free(pointer);
}

static void* memoryAllocatorResize(void* context, void* pointer, size_t size) {
	return realloc(pointer, size);
}

#define USE_SSCANF false
static void testScannerScanFormat(const char* string, size_t stringLength, char* expectedStartOfNotProcessedString,
		int expectedNumberOfSuccessfulMatches, int expectedErrorId, const char* format, ...) {
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, stringLength);

	va_list ap;
	va_start(ap, format);
	int returnedErrorId = 0;
	int numberOfSuccessfulMatches;
	if (USE_SSCANF) {
		numberOfSuccessfulMatches = vsscanf(string, format, ap);
		expectedStartOfNotProcessedString = NULL;

	} else {
		numberOfSuccessfulMatches = scannerScanFormat(&stringStreamReader.streamReader, format, ap,
			NULL, &memoryAllocatorRelease, &memoryAllocatorResize,
			&returnedErrorId);
	}
	va_end(ap);

	if (returnedErrorId != expectedErrorId) {
		printf("returnedErrorId=%d expectedErrorId=%d\n", returnedErrorId, expectedErrorId);
		assert(false);
	}

	if (numberOfSuccessfulMatches != expectedNumberOfSuccessfulMatches) {
		printf("numberOfSuccessfulMatches=%d expectedNumberOfSuccessfulMatches=%d\n", numberOfSuccessfulMatches, expectedNumberOfSuccessfulMatches);
		assert(false);
	}

	if (expectedStartOfNotProcessedString != NULL) {
		const char* startOfNotProcessedString = stringStreamReader.string + (stringLength - stringStreamReaderGetAvailable(&stringStreamReader));
		if(strcmp(startOfNotProcessedString, expectedStartOfNotProcessedString) != 0) {
			printf("startOfNotProcessedString='%s' expectedStartOfNotProcessedString='%s'\n", startOfNotProcessedString, expectedStartOfNotProcessedString);
			assert(false);
		}
	}
}

static void test6(void) {
	char outputString1[128];
	char outputString2[128];
	char outputString3[128];

	const char* inputString = "XABCDEF \t  GHIJ";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, " %s %s", outputString1, outputString2);
	assert(strcmp(outputString1, "XABCDEF") == 0);
	assert(strcmp(outputString2, "GHIJ") == 0);

	inputString = "123 \t  456   ";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, " %s %s %s", outputString1, outputString2, outputString3);
	assert(strcmp(outputString1, "123") == 0);
	assert(strcmp(outputString2, "456") == 0);

	inputString = "123 \t  456   789      ";
	testScannerScanFormat(inputString, strlen(inputString), "      ", 3, 0, " %s %s %s", outputString1, outputString2, outputString3);
	assert(strcmp(outputString1, "123") == 0);
	assert(strcmp(outputString2, "456") == 0);
	assert(strcmp(outputString3, "789") == 0);

	inputString = "123 \t  456   A \0     ";
	testScannerScanFormat(inputString, strlen(inputString), " ", 3, 0, " %s %s %s", outputString1, outputString2, outputString3);
	assert(strcmp(outputString1, "123") == 0);
	assert(strcmp(outputString2, "456") == 0);
	assert(strcmp(outputString3, "A") == 0);

	inputString = "1 | 2 | 3";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, " %s | %s | %s", outputString1, outputString2, outputString3);
	assert(strcmp(outputString1, "1") == 0);
	assert(strcmp(outputString2, "2") == 0);
	assert(strcmp(outputString3, "3") == 0);
}

static void test7(void) {
	uint32_t unsignedInt1;
	uint32_t unsignedInt2;
	uint32_t unsignedInt3;

	const char* inputString = "1 | 2 | 3";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, " %u | %u | %u", &unsignedInt1, &unsignedInt2, &unsignedInt3);
	assert(unsignedInt1 == 1);
	assert(unsignedInt2 == 2);
	assert(unsignedInt3 == 3);

	if (!USE_SSCANF) {
		inputString = "1 | 25555555333333333333333333333333333333333333333333255225555 | 3";
		testScannerScanFormat(inputString, strlen(inputString), NULL, EOF, ERANGE, " %u | %u | %u", &unsignedInt1, &unsignedInt2, &unsignedInt3);
		assert(unsignedInt1 == 1);
	}

	inputString = "{4294967295}{4294967295}{4294967295}|||";
	testScannerScanFormat(inputString, strlen(inputString), "|||", 3, 0, "{%u}{%u}{%u}", &unsignedInt1, &unsignedInt2, &unsignedInt3);
	assert(unsignedInt1 == UINT_MAX);
	assert(unsignedInt2 == UINT_MAX);
	assert(unsignedInt3 == UINT_MAX);

	inputString = "{0}\n\n\n{4294967295}\n";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, "{%u} {%u} ", &unsignedInt1, &unsignedInt2);
	assert(unsignedInt1 == 0);
	assert(unsignedInt2 == UINT_MAX);

	inputString = " {0}";
	testScannerScanFormat(inputString, strlen(inputString), " {0}", 0, 0, "{%u}", &unsignedInt1);
}

static void test8(void) {
	int32_t int1;
	int32_t int2;
	int32_t int3;

	const char* inputString = "1 | 2 | 3";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, " %d | %d | %d", &int1, &int2, &int3);
	assert(int1 == 1);
	assert(int2 == 2);
	assert(int3 == 3);

	inputString = "{2147483647}{-2147483648}{2147483647}|||";
	testScannerScanFormat(inputString, strlen(inputString), "|||", 3, 0, "{%d}{%d}{%d}", &int1, &int2, &int3);
	assert(int1 == INT_MAX);
	assert(int2 == INT_MIN);
	assert(int3 == INT_MAX);

	inputString = "{0}\n\n\n{2147483647}\n";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, "{%d} {%d} ", &int1, &int2);
	assert(int1 == 0);
	assert(int2 == INT_MAX);

	inputString = " {0}";
	testScannerScanFormat(inputString, strlen(inputString), " {0}", 0, 0, "{%d}", &int1);
}

static void test9(void) {
	int32_t int1;
	int32_t int2;
	int32_t int3;

	const char* inputString = "0xF | 077 | -3";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, " %i | %i | %i", &int1, &int2, &int3);
	assert(int1 == 15);
	assert(int2 == 63);
	assert(int3 == -3);

	inputString = "{0x7FFFFFFF}{-2147483648}{0x7FFFFFFF}|||";
	testScannerScanFormat(inputString, strlen(inputString), "|||", 3, 0, "{%i}{%i}{%i}", &int1, &int2, &int3);
	assert(int1 == INT_MAX);
	assert(int2 == INT_MIN);
	assert(int3 == INT_MAX);

	inputString = "{0x10}\n\n\n{5678}\n";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, "{%i} {%i} ", &int1, &int2);
	assert(int1 == 16);
	assert(int2 == 5678);

	inputString = " {0}";
	testScannerScanFormat(inputString, strlen(inputString), " {0}", 0, 0, "{%i}", &int1);
}

static void test10(void) {
	int32_t int1;
	int32_t int2;
	int32_t int3;

	const char* inputString = "0xF | -ABC | 1";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, " %X | %x | %X", &int1, &int2, &int3);
	assert(int1 == 0xF);
	assert(int2 == -0xABC);
	assert(int3 == 0x1);

	inputString = "{0x10}\n\n\n{5678}\n";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, "{%X} {%x} ", &int1, &int2);
	assert(int1 == 0x10);
	assert(int2 == 0x5678);
}

static void test11(void) {
	char char1;
	char char2;
	char char3;

	const char* inputString = "ABC";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, "%c%c%c", &char1, &char2, &char3);
	assert(char1 == 'A');
	assert(char2 == 'B');
	assert(char3 == 'C');

	inputString = "   ABC";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, " %c%c%c", &char1, &char2, &char3);
	assert(char1 == 'A');
	assert(char2 == 'B');
	assert(char3 == 'C');

	inputString = "%A%B|C ";
	testScannerScanFormat(inputString, strlen(inputString), "|C ", 2, 0, " %%%c%%%c%%%c", &char1, &char2, &char3);
	assert(char1 == 'A');
	assert(char2 == 'B');
}

static void test12(void) {
	uint32_t int1;

	const char* inputString = "q";
	testScannerScanFormat(inputString, strlen(inputString), "q", 0, 0, "%u", &int1);
}

static void test13(void) {
	char outputString1[128];
	char outputString2[128];

	const char* inputString = " XABCDEF J";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, "%s %s", outputString1, outputString2);
	assert(strcmp(outputString1, "XABCDEF") == 0);
	assert(strcmp(outputString2, "J") == 0);
}

static void test14(void) {
	int int1;

	const char* inputString = "   123";
	testScannerScanFormat(inputString, strlen(inputString), "", 1, 0, "%d", &int1);
	assert(int1 == 123);
}

static void test15(void) {
	char* string1;
	char* string2;
	char* string3;

	const char* inputString = "   XABCDEF";
	testScannerScanFormat(inputString, strlen(inputString), "", 1, 0, "%ms", &string1);
	assert(strcmp(string1, "XABCDEF") == 0);
	free(string1);

	inputString = "   XABCDEF   AAA    AB";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, "%ms%ms%ms", &string1, &string2, &string3);
	assert(strcmp(string1, "XABCDEF") == 0);
	assert(strcmp(string2, "AAA") == 0);
	assert(strcmp(string3, "AB") == 0);
	free(string1);
	free(string2);
	free(string3);
}

static void test16(void) {
	char* string1;

	const char* inputString = "   \xFF\xFE\xFD";
	testScannerScanFormat(inputString, strlen(inputString), "", 1, 0, "%ms", &string1);
	assert(strcmp(string1, "\xFF\xFE\xFD") == 0);
	free(string1);
}

static void test17(void) {
	const char* inputString;

	inputString = "  C";
	testScannerScanFormat(inputString, strlen(inputString), "  C", 0, 0, "C");

	inputString = "  C";
	testScannerScanFormat(inputString, strlen(inputString), "", EOF, 0, " C");
}

static void testScannerFloat(const char* string, bool allowSpaceBefore, const char* expectedStartOfNotProcessedString, long double expectedOutput, int expectedErrorId) {
	const char* startOfNotProcessedString;
	long double output;

	int returnedErrorId = scannerParseFloat(string, allowSpaceBefore, expectedStartOfNotProcessedString != NULL ? &startOfNotProcessedString : NULL, &output);
	if (returnedErrorId != expectedErrorId) {
		printf("returnedErrorId=%d expectedErrorId=%d\n", returnedErrorId, expectedErrorId);
		assert(false);
	}

	if (isnan(expectedOutput)) {
		if (!isnan(output) || signbit(expectedOutput) != signbit(output)) {
			printf("output=%Lf expectedOutput=%Lf\n", output, expectedOutput);
			assert(false);
		}

	} else if (isinf(expectedOutput)) {
		if (!isinf(output) || signbit(expectedOutput) != signbit(output)) {
			printf("output=%Lf expectedOutput=%Lf\n", output, expectedOutput);
			assert(false);
		}

	} else if (!mathUtilsApproximatelyEquals(output, expectedOutput, 0.001)) {
		printf("output=%Lf expectedOutput=%Lf\n", output, expectedOutput);
		assert(false);
	}
	if (expectedStartOfNotProcessedString != NULL) {
		if(strcmp(startOfNotProcessedString, expectedStartOfNotProcessedString) != 0) {
			printf("startOfNotProcessedString='%s' expectedStartOfNotProcessedString='%s'\n", startOfNotProcessedString, expectedStartOfNotProcessedString);
			assert(false);
		}
	}
}

static void test18(void) {
	testScannerFloat("nan", true, "", NAN, 0);
	testScannerFloat("nAn", true, "", NAN, 0);
	testScannerFloat("   nAn", true, "", NAN, 0);
	testScannerFloat("-nAn", true, "", -NAN, 0);
	testScannerFloat("-NAnA", true, "A", -NAN, 0);

	testScannerFloat("inf", true, "", INFINITY, 0);
	testScannerFloat("inF", true, "", INFINITY, 0);
	testScannerFloat("-Inf", true, "", -INFINITY, 0);
	testScannerFloat("    -Inf", true, "", -INFINITY, 0);
	testScannerFloat("   -infB", true, "B", -INFINITY, 0);
	testScannerFloat("-infA", true, "A", -INFINITY, 0);
	testScannerFloat("-infinityA", true, "A", -INFINITY, 0);
	testScannerFloat("-infinitO", true, "initO", -INFINITY, 0);

	testScannerFloat("-inA", true, "-inA", 0, 0);
	testScannerFloat(" -inA", true, " -inA", 0, 0);
	testScannerFloat(" - inA", true, " - inA", 0, 0);
	testScannerFloat(" Inf", false, " Inf", 0, 0);
	testScannerFloat("", false, "", 0, 0);
	testScannerFloat("  ", false, "  ", 0, 0);
	testScannerFloat("\n\n  na", true, "\n\n  na", 0, 0);
}

static void test19(void) {
	testScannerFloat("1234", true, "", 1234, 0);
	testScannerFloat("0001234", true, "", 1234, 0);
	testScannerFloat("1.234", true, "", 1.234, 0);
	testScannerFloat("12.34", true, "", 12.34, 0);
	testScannerFloat("1234.", true, "", 1234, 0);
	testScannerFloat("1234.5678", true, "", 1234.5678, 0);
	testScannerFloat("   1234.56789XXX", true, "XXX", 1234.5678, 0);
	testScannerFloat("   1234P56789XXX", true, "P56789XXX", 1234, 0);

	testScannerFloat("13214871023984710982374091827348917230984710298374019823740198237401982734091874009187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928735689304568305860398506938450968409983605861984570198750928734509827345092873450928734509283475029834570298347502983475234098192837491837413214871023984710982374091827348917230984710298374019823740198237401982734091874009187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928735689304568305860398506938450968409983605861984570198750928734509827345092873450928734509283475029834570298347502983475234098192837491831321487102398471098237409182734891723098471029837401982374019823740198273409187400918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873568930456830586039850693845096840998360586198457019875092873450982734509287345092873450928347502983457029834750298347523409819283749183132148710239847109823740918273489172309847102983740198237401982374019827340918740091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287356893045683058603985069384509684099836058619845701987509287345098273450928734509287345092834750298345702983475029834752340981928374918313214871023984710982374091827348917230984710298374019823740198237401982734091874009187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928735689304568305860398506938450968409983605861984570198750928734509827345092873450928734509283475029834570298347502983475234098192837491831321487102398471098237409182734891723098471029837401982374019823740198273409187400918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873568930456830586039850693845096840998360586198457019875092873450982734509287345092873450928347502983457029834750298347523409819283749183132148710239847109823740918273489172309847102983740198237401982374019827340918740091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287356893045683058603985069384509684099836058619845701987509287345098273450928734509287345092834750298345702983475029834752340981928374918313214871023984710982374091827348917230984710298374019823740198237401982734091874009187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928735689304568305860398506938450968409983605861984570198750928734509827345092873450928734509283475029834570298347502983475234098192837491831321487102398471098237409182734891723098471029837401982374019823740198273409187400918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873568930456830586039850693845096840998360586198457019875092873450982734509287345092873450928347502983457029834750298347523409819283749183132148710239847109823740918273489172309847102983740198237401982374019827340918740091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287356893045683058603985069384509684099836058619845701987509287345098273450928734509287345092834750298345702983475029834752340981928374918313214871023984710982374091827348917230984710298374019823740198237401982734091874009187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928735689304568305860398506938450968409983605861984570198750928734509827345092873450928734509283475029834570298347502983475234098192837491831321487102398471098237409182734891723098471029837401982374019823740198273409187400918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873091874056893045683058603985069384509684099836058619845701987509287345098273450928730918740568930456830586039850693845096840998360586198457019875092873450982734509287309187405689304568305860398506938450968409983605861984570198750928734509827345092873568930456830586039850693845096840998360586198457019875092873450982734509287345092873450928347502983457029834750298347523409819283749183",
				true, "", INFINITY, ERANGE);
	testScannerFloat("1.79E-233308", true, "", 0, ERANGE);
	testScannerFloat("1.79E+233308", true, "", INFINITY, ERANGE);

	testScannerFloat("1234P123", true, "P123", 1234, 0);
	testScannerFloat("1234E1", true, "", 12340, 0);
	testScannerFloat("1234E-1", true, "", 123.4, 0);
	testScannerFloat("1234E-3XXX", true, "XXX", 1.234, 0);
	testScannerFloat("1..23", true, ".23", 1, 0);
	testScannerFloat("5E.23", true, "E.23", 5, 0);
	testScannerFloat("5ABCDEF", true, "ABCDEF", 5, 0);
	testScannerFloat("1.234567E+1", true, "", 12.34567, 0);

	testScannerFloat(" +.E2", true, " +.E2", 0, 0);
	testScannerFloat(".XXX", true, ".XXX", 0, 0);
	testScannerFloat(" .XXX", true, " .XXX", 0, 0);

	testScannerFloat(" 0x10 ", true, " ", 16, 0);
	testScannerFloat("0x10", true, "", 16, 0);
	testScannerFloat("0x11P1", true, "", 34, 0);
	testScannerFloat("0x11P-1", true, "", 8.5, 0);
	testScannerFloat("0x11E", true, "", 286, 0);
	testScannerFloat("0xA.A", true, "", 10.625, 0);
}

static void test20(void) {
	float f;
	double d;
	long double ld;
	int i;
	char* string;

	const char* inputString;

	inputString = "1.234 123 text!";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, "%f%d%ms", &d, &i, &string);
	assert(mathUtilsApproximatelyEquals(d, 1.234, DELTA));
	assert(i == 123);
	assert(strcmp(string, "text!") == 0);
	free(string);

	inputString = "text: 1234.56789";
	testScannerScanFormat(inputString, strlen(inputString), "", 2, 0, "%ms%Lf", &string, &ld);
	assert(strcmp(string, "text:") == 0);
	assert(mathUtilsApproximatelyEquals(ld, 1234.56789, DELTA));
	free(string);

	inputString = "3.14 text 1234.00089";
	testScannerScanFormat(inputString, strlen(inputString), "", 3, 0, "%f%ms%Lf", &f, &string, &ld);
	assert(mathUtilsApproximatelyEquals(f, 3.14, DELTA));
	assert(strcmp(string, "text") == 0);
	assert(mathUtilsApproximatelyEquals(ld, 1234.00089, DELTA));
	free(string);

	inputString = "1.2345text";
	testScannerScanFormat(inputString, strlen(inputString), "text", 1, 0, "%f", &f);
	assert(mathUtilsApproximatelyEquals(f, 1.2345, DELTA));
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();
	test8();
	test9();
	test10();
	test11();
	test12();
	test13();
	test14();
	test15();
	test16();
	test17();
	test18();
	test19();
	test20();

	return 0;
}
