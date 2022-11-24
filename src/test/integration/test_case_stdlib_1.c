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
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "util/math_utils.h"

#include "test/integration_test.h"

static int reverseIntegerComparator(const int* item1, const int* item2) {
	return *item2 - *item1;
}

static void testQsort(void) {
	const int arrayLength = 20;
	int* array = calloc(sizeof(int), arrayLength);
	assert(array != NULL);

	array[0] = -842;
	array[1] = 1984;
	array[2] = 2643;
	array[3] = -4820;
	array[4] = 4326;
	array[5] = -1300;
	array[6] = 585;
	array[7] = -2767;
	array[8] = 2081;
	array[9] = -3486;
	array[10] = 3115;
	array[11] = 2476;
	array[12] = 2696;
	array[13] = -1602;
	array[14] = 4980;
	array[15] = 0;
	array[16] = 0;
	array[17] = -5000;
	array[18] = 1000;
	array[19] = -1000;

	qsort(array, arrayLength, sizeof(int), (int (*)(const void*, const void*))  &reverseIntegerComparator);

	assert(array[19] == -5000);
	assert(array[18] == -4820);
	assert(array[17] == -3486);
	assert(array[16] == -2767);
	assert(array[15] == -1602);
	assert(array[14] == -1300);
	assert(array[13] == -1000);
	assert(array[12] == -842);
	assert(array[11] == 0);
	assert(array[10] == 0);
	assert(array[9] == 585);
	assert(array[8] == 1000);
	assert(array[7] == 1984);
	assert(array[6] == 2081);
	assert(array[5] == 2476);
	assert(array[4] == 2643);
	assert(array[3] == 2696);
	assert(array[2] == 3115);
	assert(array[1] == 4326);
	assert(array[0] == 4980);

	free(array);
}

static int reverseStringComparator(const char** item1, const char** item2, void* nullFirst) {
	if (*item1 == NULL && *item2 == NULL) {
		return 0;

	} else if (*item1 == NULL) {
		return nullFirst ? -1 : 1;

	} else if (*item2 == NULL) {
		return nullFirst ? 1 : -1;

	} else {
		return strcmp(*item1, *item2);
	}
}

static void testQsort_r(void) {
	const int arrayLength = 10;
	const char** array = calloc(sizeof(const char*), arrayLength);
	assert(array != NULL);

	array[0] = "James";
	array[1] = NULL;
	array[2] = NULL;
	array[3] = "Robert";
	array[4] = "John";
	array[5] = NULL;
	array[6] = "William";
	array[7] = "David";
	array[8] = NULL;
	array[9] = "Michael";

	qsort_r(array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*, void*))  &reverseStringComparator, (void*) (int) true);

	assert(array[0] == NULL);
	assert(array[1] == NULL);
	assert(array[2] == NULL);
	assert(array[3] == NULL);
	assert(strcmp(array[4], "David") == 0);
	assert(strcmp(array[5], "James") == 0);
	assert(strcmp(array[6], "John") == 0);
	assert(strcmp(array[7], "Michael") == 0);
	assert(strcmp(array[8], "Robert") == 0);
	assert(strcmp(array[9], "William") == 0);

	qsort_r(array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*, void*))  &reverseStringComparator, (void*) (int) false);

	assert(strcmp(array[0], "David") == 0);
	assert(strcmp(array[1], "James") == 0);
	assert(strcmp(array[2], "John") == 0);
	assert(strcmp(array[3], "Michael") == 0);
	assert(strcmp(array[4], "Robert") == 0);
	assert(strcmp(array[5], "William") == 0);
	assert(array[6] == NULL);
	assert(array[7] == NULL);
	assert(array[8] == NULL);
	assert(array[9] == NULL);

	free(array);
}

static int stringComparator(const char** item1, const char** item2) {
	return strcmp(*item1, *item2);
}

static void testBsearch(void) {
	const int arrayLength = 10;
	const char** array = calloc(sizeof(const char*), arrayLength);
	assert(array != NULL);

	array[0] = "taste";
	array[1] = "either";
	array[2] = "drink";
	array[3] = "fishing";
	array[4] = "accuse";
	array[5] = "land";
	array[6] = "reject";
	array[7] = "milk";
	array[8] = "cast";
	array[9] = "unusual";

	qsort(array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*))  &stringComparator);

	assert(strcmp(array[0], "accuse") == 0);
	assert(strcmp(array[1], "cast") == 0);
	assert(strcmp(array[2], "drink") == 0);
	assert(strcmp(array[3], "either") == 0);
	assert(strcmp(array[4], "fishing") == 0);
	assert(strcmp(array[5], "land") == 0);
	assert(strcmp(array[6], "milk") == 0);
	assert(strcmp(array[7], "reject") == 0);
	assert(strcmp(array[8], "taste") == 0);
	assert(strcmp(array[9], "unusual") == 0);

	const char* element;
	const char** result;

	element = "land";
	result = bsearch(&element, array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*))  &stringComparator);
	assert(result != NULL);
	assert(strcmp(*result, element) == 0);

	element = "123";
	result = bsearch(&element, array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*))  &stringComparator);
	assert(result == NULL);

	element = "unusual";
	result = bsearch(&element, array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*))  &stringComparator);
	assert(result != NULL);
	assert(strcmp(*result, element) == 0);

	element = "accuse";
	result = bsearch(&element, array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*))  &stringComparator);
	assert(result != NULL);
	assert(strcmp(*result, element) == 0);

	element = "";
	result = bsearch(&element, array, arrayLength, sizeof(const char*), (int (*)(const void*, const void*))  &stringComparator);
	assert(result == NULL);

	free(array);
}

static void testStrtoul(void) {
   unsigned int result;
   char* string;

   string = NULL;
   errno = 0;
   result = strtoul("ABC", &string, 10);
   assert(errno == 0);
   assert(strcmp(string, "ABC") == 0);
   assert(result == 0);

   string = NULL;
   errno = 0;
   result = strtoul("0x123", &string, 0);
   assert(errno == 0);
   assert(strcmp(string, "") == 0);
   assert(result == 0x123);

   string = NULL;
   errno = 0;
   result = strtoul("", &string, 0);
   assert(errno == 0);
   assert(strcmp(string, "") == 0);
   assert(result == 0);

   string = NULL;
   errno = 0;
   result = strtoul("     123", &string, 0);
   assert(errno == 0);
   assert(strcmp(string, "") == 0);
   assert(result == 123);

   string = NULL;
   errno = 0;
   result = strtoul("+  \n   123", &string, 0);
   assert(errno == 0);
   assert(strcmp(string, "+  \n   123") == 0);
   assert(result == 0);

   string = NULL;
   errno = 0;
   result = strtoul("  +456", &string, 10);
   assert(errno == 0);
   assert(strcmp(string, "") == 0);
   assert(result == 456);

   string = NULL;
   errno = 0;
   result = strtoul("  -3", &string, 10);
   assert(errno == 0);
   assert(strcmp(string, "") == 0);
   assert(result == 4294967293);

   string = NULL;
   errno = 0;
   result = strtoul("  +010", &string, 0);
   assert(errno == 0);
   assert(strcmp(string, "") == 0);
   assert(result == 8);

   string = NULL;
   errno = 0;
   result = strtoul("   0546 ", &string, 10);
   assert(errno == 0);
   assert(strcmp(string, " ") == 0);
   assert(result == 546);

   string = NULL;
   errno = 0;
   result = strtoul("   0x546 ", &string, 10);
   assert(errno == 0);
   assert(strcmp(string, "x546 ") == 0);
   assert(result == 0);
}

static void testMkstemp(void) {
	const char* string = "This is a write testing!";

	char* path = strdup("/tmp/testXXXXXX");
	assert(path != NULL);
	int fileDescriptorIndex = mkstemp(path);
	assert(fileDescriptorIndex >= 0);

	FILE* stream = fdopen(fileDescriptorIndex, "w");
	assert(stream != NULL);
	fputs(string, stream);
	fclose(stream);

	stream = fopen(path, "r");
	assert(stream != NULL);
	const size_t bufferSize = 128;
	char buffer[bufferSize];
	size_t result = fread(buffer, 1, bufferSize, stream);
	assert(result == strlen(string));
	buffer[result] = '\0';
	assert(strcmp(string, buffer) == 0);
	fclose(stream);

	free(path);
}

static void testStrtod(void) {
	const char* string;
	char* startOfNotProcessedString;
	double result;

	string = "1.234E1";
	result = strtod(string, &startOfNotProcessedString);
	assert(strcmp(startOfNotProcessedString, "") == 0);
	assert(mathUtilsApproximatelyEquals(1.234E1, result, 0.001));

	string = "nan";
	result = strtod(string, &startOfNotProcessedString);
	assert(strcmp(startOfNotProcessedString, "") == 0);
	assert(isnan(result));

	string = "  150.123AA";
	result = strtod(string, &startOfNotProcessedString);
	assert(strcmp(startOfNotProcessedString, "AA") == 0);
	assert(mathUtilsApproximatelyEquals(150.123, result, 0.001));

	string = "  0x4P1__";
	result = strtod(string, &startOfNotProcessedString);
	assert(strcmp(startOfNotProcessedString, "__") == 0);
	assert(mathUtilsApproximatelyEquals(8, result, 0.001));

	string = "   0x4p2__";
	result = strtod(string, &startOfNotProcessedString);
	assert(strcmp(startOfNotProcessedString, "__") == 0);
	assert(mathUtilsApproximatelyEquals(16, result, 0.001));

	string = "A__1";
	result = strtod(string, &startOfNotProcessedString);
	assert(strcmp(startOfNotProcessedString, "A__1") == 0);
	assert(mathUtilsApproximatelyEquals(0, result, 0.001));
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testQsort();
	testQsort_r();
	testBsearch();
	testStrtoul();
	testMkstemp();
	testStrtod();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
