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
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "util/string_utils.h"

void* memcpy(void* destination, const void* source, size_t count) {
	void* originalDestination = destination;
	__asm__ __volatile__(
		"cld;"
		"rep movsb;"
		: "=D"(destination), "=S"(source), "=c"(count)
		: "D"(destination), "S"(source), "c"(count)
		: "memory");
	return originalDestination;
}

void* mempcpy(void* destination, const void* source, size_t count) {
	return memcpy(destination, source, count) + count;
}

void* memmove(void* destination, const void* source, size_t count) {
	if (count > 0) {
		void* originalDestination = destination;
		/*
		 * There are four different cases considering data copying between two buffers. Only one requires copying data backward.
		 * Is this the case?
		 */
		if (source < destination && source + count > destination) {
			__asm__ __volatile__(
				"std;"
				"rep movsb;"
				: "=D"(destination), "=S"(source), "=c"(count)
				: "D"(destination + (count - 1)), "S"(source + (count - 1)), "c"(count)
				: "memory");
			return originalDestination;

		} else {
			return memcpy(destination, source, count);
		}

	} else {
		return destination;
	}
}

int memcmp(const void* source1, const void* source2, size_t count) {
	int result = 0;
	if (count > 0) {
		__asm__ __volatile__(
			"cld;"
			"repe cmpsb;"
			"movl $0, %%eax;"
			"movl $0, %%ecx;"
			"setg %%al;"
			"setl %%cl;"
			"subl %%eax, %%ecx;"
			: "=D"(source1), "=S"(source2), "=c"(result)
			: "D"(source1), "S"(source2), "c"(count)
			: "eax", "cc", "memory");
	}
	return result;
}

void* memset(void *pointer, int value, size_t count) {
	void* originalPointer = pointer;
	__asm__ __volatile__(
		"cld;"
		"rep stosb"
		: "=D"(pointer), "=c"(count)
		: "D"(pointer), "c"(count), "a"(value)
		: "memory");
	return originalPointer;
}

void* memset16(void* pointer, int value, size_t count) {
	void* originalPointer = pointer;
	__asm__ __volatile__(
		"cld;"
		"rep stosw;"
		: "=D"(pointer), "=c"(count)
		: "D"(pointer), "c"(count), "a"(value)
		: "memory");
	return originalPointer;
}

size_t strlen(const char* string) {
	size_t size = UINT_MAX;
	__asm__ __volatile__(
		"movl $0, %%eax;"
		"cld;"
		"repne scasb;"
		: "=D"(string), "=c"(size)
		: "D"(string), "c"(size)
		: "eax", "cc", "memory");
	return UINT_MAX - size - 1;
}

size_t strnlen(const char* string, size_t maxLength) {
	size_t size = 0;
	while (size < maxLength && string[size] != '\0') {
		size++;
	}

	return size;
}

size_t strlcpy(char* destination, const char* source, size_t destinationLength) {
	assert(destinationLength >= 1);

	size_t i;
	size_t sourceLength = strlen(source);
	for(i = 0; i < destinationLength - 1 && i < sourceLength; i++) {
		destination[i] = source[i];
	}
	destination[i] = '\0';
	return sourceLength;
}

char* strncpy(char* destination, const char* source, size_t destinationLength) {
	char c;
	int i;
	for(i = 0; i < destinationLength && (c = source[i]) != '\0'; i++) {
		destination[i] = c;
	}
	for(; i < destinationLength; i++) {
		destination[i] = '\0';
	}

	return destination;
}

char* strcpy(char* destination, const char* source) {
	char c;
	int i = 0;
	do {
		c = source[i];
		destination[i] = c;
		i++;
	} while (c != '\0');

	return destination;
}

char* stpcpy(char* destination, const char* source) {
	char c;
	int i = 0;
	do {
		c = source[i];
		destination[i] = c;
		i++;
	} while (c != '\0');

	return destination + (i - 1);
}

int strcmp(const char* string1, const char* string2) {
	return strncmp(string1, string2, UINT_MAX);
}

int strncmp(const char* string1, const char* string2, size_t count) {
	size_t i = 0;
	char c1;
	char c2;
	do {
		c1 = string1[i];
		c2 = string2[i];
		i++;
	} while (c1 != '\0' && c2 != '\0' && c1 == c2 && i < count);

	return (int)c1 - (int)c2;
}

int strcasecmp(const char* string1, const char* string2) {
	return strncasecmp(string1, string2, UINT_MAX);
}

int strncasecmp(const char* string1, const char* string2, size_t count) {
	size_t i = 0;
	char c1;
	char c2;
	do {
		c1 = tolower(string1[i]);
		c2 = tolower(string2[i]);
		i++;
	} while (c1 != '\0' && c2 != '\0' && c1 == c2 && i < count);

	return (int)c1 - (int)c2;
}

static char* sharedContext;
char* strtok(char* string, const char* delimiter) {
	return strtok_r(string, delimiter, &sharedContext);
}

char* strtok_r(char* string, const char* delimiter, char** context) {
	return stringUtilsParseTokens(string, delimiter, context);
}

char* strchr(const char* string, int character) {
	do {
		if (*string == (char) character) {
			return (char*) string;
		}
		string++;
	} while (*string);

	return NULL;
}

char* strrchr(const char* string, int character) {
	int index = strlen(string);

	do {
		if (string[index] == (char) character) {
			return (char*) &string[index];
		}
	} while (--index >= 0);

	return NULL;
}

void* memchr(const void* string, int character, size_t size) {
	const unsigned char* castedBuffer = string;
	for (size_t count = 0; count < size; count++) {
		if (*castedBuffer == character) {
			return (void*) castedBuffer;
		}
		castedBuffer++;
	}

	return NULL;
}

void* memrchr(const void* string, int character, size_t size) {
	const unsigned char* castedBuffer = string + size;
	while(castedBuffer != string) {
		if (*castedBuffer == character) {
			return (void*) castedBuffer;
		}
		castedBuffer--;
	}

	return NULL;
}

char* strcat(char* destination, const char* source) {
	return strncat(destination, source, UINT_MAX);
}

char* strncat(char* destination, const char* source, size_t sourceLength) {
	size_t destinationLength = strlen(destination);

	char character;
	int i;
	for (i = 0; i < sourceLength && (character = source[i]) != '\0'; i++) {
		destination[destinationLength + i] = character;
	}
	destination[destinationLength + i] = '\0';

	return destination;
}

/* Very simple and inefficient version. */
char* strstr(const char* string, const char* substring) {
	size_t length = strlen(substring);
	const char* substringCandidate = string;
	while (*substringCandidate != '\0') {
		if (strncmp(substringCandidate, substring, length) == 0) {
			return (char*) substringCandidate;
		}
		substringCandidate++;
	}

	return NULL;
}

char* strpbrk(const char* string, const char* characters) {
	const char* substringCandidate = string;
	while (*substringCandidate != '\0') {
		if (strchr(characters, *substringCandidate) != NULL) {
			return (char*) substringCandidate;
		}

		substringCandidate++;
	}

	return NULL;
}

static size_t commonStringSpan(const char* string, const char* characters, bool areAcceptedCharacters) {
	size_t result = 0;
	while (*string) {
		const char* nextCharacter = characters;

		bool include = false;
		while (*nextCharacter) {
			if (*string == *nextCharacter) {
				include = true;
				break;
			}
			nextCharacter++;
		}

		if ((include && areAcceptedCharacters) || (!include && !areAcceptedCharacters)) {
			result++;
		} else {
			break;
		}

		string++;
	}

	return result;
}

size_t strspn(const char* string, const char* acceptedCharacters) {
	return commonStringSpan(string, acceptedCharacters, true);
}

size_t strcspn(const char* string, const char* rejectedCharacters) {
	return commonStringSpan(string, rejectedCharacters, false);
}
