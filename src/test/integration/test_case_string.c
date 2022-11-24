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
#include <stdio.h>
#include <string.h>

#include "test/integration_test.h"

static void testStrcat(void) {
	const int bufferSize = 128;
	char buffer[bufferSize];

	assert(buffer == strcpy(buffer, "ABC"));
	assert(strcmp("ABC", buffer) == 0);

	assert(buffer == strcat(buffer, "DEF"));
	assert(strcmp("ABCDEF", buffer) == 0);

	assert(buffer == strncat(buffer, "GHIJKLM", 3));
	assert(strcmp("ABCDEFGHI", buffer) == 0);
}

static void testStrstr(void) {
	const char* string;
	const char* substring;
	char* result;

	string = "AAABBBCCCDDD";
	substring = "BBC";
	result = strstr(string, substring);
	assert(result != NULL);
	assert(strncmp(&string[4], substring, strlen(substring)) == 0);

	string = "";
	substring = "AAABBBCCC";
	result = strstr(string, substring);
	assert(result == NULL);

	string = "123";
	substring = "BBC";
	result = strstr(string, substring);
	assert(result == NULL);
}

static void testStrchr(void) {
	const char* string;
	char* result;

	string = "AA""\xFF""123";
	result = strchr(string, '\xFF');
	assert(result != NULL);
	assert(strcmp(result, "\xFF""123") == 0);

	string = "AA""\xFF""123";
	result = strchr(string, '\xFE');
	assert(result == NULL);

}

static void testStrrchr(void) {
	const char* string;
	char* result;

	string = "AA""\xFF""123";
	result = strrchr(string, '\xFF');
	assert(result != NULL);
	assert(strcmp(result, "\xFF""123") == 0);

	string = "AA""\xFF""123";
	result = strrchr(string, '\xFE');
	assert(result == NULL);

	string = "AA______123";
	result = strrchr(string, '_');
	assert(result != NULL);
	assert(strcmp(result, "_123") == 0);
}

static void testMemchr(void) {
	const uint8_t memory[] = {0, 0, 255, 15, 254, 0};
	void* result;

	result = memchr(memory, 254, 4);
	assert(result == NULL);

	result = memchr(memory, 255, 3);
	assert(result == &memory[2]);

	result = memchr(memory, 0, 6);
	assert(result == &memory[0]);

	result = memchr(memory, 0, 0);
	assert(result == NULL);

	result = memchr(memory, 254, 6);
	assert(result == &memory[4]);
}

static void testStrtok(void) {
	char string[256];
	char* result;

	strcpy(string, "AA""\xFF""BB""\xFF""CC""\xFF");
	result = strtok(string, "\xFF");
	assert(strcmp("AA", result) == 0);

	result = strtok(NULL, "\xFF");
	assert(strcmp("BB", result) == 0);

	result = strtok(NULL, "\xFF");
	assert(strcmp("CC", result) == 0);

	result = strtok(NULL, "\xFF");
	assert(result == NULL);
}

static void testStrnlen(void) {
	char string[256];

	strcpy(string, "AA""\xFF""BB""\xFF""CC""\xFF");
	assert(strnlen(string, UINT_MAX) == 9);

	strcpy(string, "");
	assert(strnlen(string, UINT_MAX) == 0);

	assert(strnlen(NULL, 0) == 0);

	strcpy(string, "ABC");
	assert(strnlen(string, 2) == 2);
}

static void testMemcpy(void) {
	const char* string = "Testing...";
	char stringCopy[64];

	void* result = memcpy(stringCopy, string, strlen(string) + 1);
	assert(result == stringCopy);
	assert(strcmp(string, stringCopy) == 0);
}

static void testMemmove(void) {
	const char* string = "Testing again...";
	const int bufferSize = 64;
	char buffer[bufferSize];

	void* result = memset(buffer, 0, bufferSize);
	assert(result == buffer);

	result = memmove(buffer + bufferSize - (strlen(string) + 1), string, strlen(string) + 1);
	assert(strcmp(result, string) == 0);

	result = memmove(result - 8, result, strlen(string) + 1);
	assert(strcmp(result, string) == 0);

	assert(memcmp(result, "Testing again...\0gain...", strlen(string) + 8 + 1) == 0);

	assert(buffer[bufferSize - 1] == '\0');
	assert(buffer[bufferSize - 2] == '.');
	assert(buffer[bufferSize - 3] == '.');
	assert(buffer[bufferSize - 4] == '.');
	assert(buffer[bufferSize - 5] == 'n');
	assert(buffer[bufferSize - 6] == 'i');
	assert(buffer[bufferSize - 7] == 'a');
	assert(buffer[bufferSize - 8] == 'g');
	assert(buffer[bufferSize - 9] == '\0');
}

static void testStrspn(void) {
	const char* string;
	const char* acceptedCharacters;
	size_t result;

	string = "abccbaXYZ";
	acceptedCharacters = "cba";
	result = strspn(string, acceptedCharacters);
	assert(result == 6);

	string = "";
	acceptedCharacters = "cba";
	result = strspn(string, acceptedCharacters);
	assert(result == 0);

	string = "XYZabccba";
	acceptedCharacters = "cba";
	result = strspn(string, acceptedCharacters);
	assert(result == 0);

	string = "XYZabccba";
	acceptedCharacters = "";
	result = strspn(string, acceptedCharacters);
	assert(result == 0);

	string = "0123324535";
	acceptedCharacters = "0123456789";
	result = strspn(string, acceptedCharacters);
	assert(result == 10);

	string = "010110XYZ";
	acceptedCharacters = "01";
	result = strspn(string, acceptedCharacters);
	assert(result == 6);
}

static void testStrcspn(void) {
	const char* string;
	const char* rejectedCharacters;
	size_t result;

	string = "XYZ0123";
	rejectedCharacters = "0123456789";
	result = strcspn(string, rejectedCharacters);
	assert(result == 3);

	string = "ABCDEF";
	rejectedCharacters = "";
	result = strcspn(string, rejectedCharacters);
	assert(result == 6);

	string = "";
	rejectedCharacters = "";
	result = strcspn(string, rejectedCharacters);
	assert(result == 0);

	string = "";
	rejectedCharacters = "0123456789";
	result = strcspn(string, rejectedCharacters);
	assert(result == 0);

	string = "ABCD";
	rejectedCharacters = "0123456789";
	result = strcspn(string, rejectedCharacters);
	assert(result == 4);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testStrcat();
	testStrstr();
	testStrchr();
	testStrrchr();
	testMemchr();
	testStrtok();
	testStrnlen();
	testMemcpy();
	testMemmove();
	testStrspn();
	testStrcspn();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
