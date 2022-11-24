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

#ifndef STRING_H
	#define STRING_H

	#include <stdbool.h>
	#include <stdint.h>
	#include <stdlib.h>

	#ifndef KERNEL_CODE
		const char* strsignal(int signalId);
	#endif

	void* memset16(void* pointer, int value, size_t count);
	void* memset(void *pointer, int value, size_t count);
	int memcmp(const void* source1, const void* source2, size_t count);
	void* memmove(void*, const void*, size_t);
	void* memcpy(void* destination, const void* source, size_t count);
   void* mempcpy(void* destination, const void* source, size_t count);

	size_t strlen(const char* string);
	size_t strnlen(const char* string, size_t maxLength);

	char* stpcpy(char* destination, const char* source);
	char* strcpy(char* destination, const char* source);
	char* strncpy(char* destination, const char* source, size_t destinationLength);
	size_t strlcpy(char* destination, const char* source, size_t destinationLength);

	int strcmp(const char *string1, const char* string2);
	int strncmp(const char* string1, const char* string2, size_t count);
	int strcasecmp(const char* string1, const char* string2);
	int strncasecmp(const char* string1, const char* string2, size_t count);

	char* strtok(char* string, const char* delimiter);
	char* strtok_r(char* string, const char* delimiter, char** context);

	char* strdup(const char* string);
	char* strndup(const char* string, size_t size);

	char* strerror(int errnum);

	char* strcat(char* destination, const char* source);
	char* strncat(char* destination, const char* source, size_t sourceLength);

	char* strstr(const char* string, const char* substring);
	char* strchr(const char* string, int character);
	char* strrchr(const char* string, int character);
	char* strpbrk(const char* string, const char* characters);

	void* memchr(const void* string, int character, size_t size);
	void* memrchr(const void* string, int character, size_t size);

	#define strcoll(string1, string2) strcmp(string1, string2)

	size_t strspn(const char* string, const char* acceptedCharacters);
	size_t strcspn(const char* string, const char* rejectedCharacters);

#endif
