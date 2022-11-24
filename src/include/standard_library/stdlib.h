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

#ifndef STDLIB_H
	#define STDLIB_H

	#include <stddef.h>
	#include <stdint.h>

	#define EXIT_SUCCESS 0
   #define EXIT_FAILURE 1

	#define MB_CUR_MAX 1

	#ifndef KERNEL_CODE
		#include <wchar.h>

		void* malloc(size_t size);
		void* realloc(void* pointer, size_t);
		void* calloc(size_t elementSize, size_t elementCount);
		void free(void* pointer);

		void abort(void);
		void __attribute__ ((cdecl)) exit(int status);
      int atexit(void (*callback)(void));

		int atoi(const char* string);
		long int strtol(const char* string, char** startOfNotProcessedString, int base);
		unsigned long strtoul(const char* string, char** startOfNotProcessedString, int base);
      double atof(const char* string);
		double strtod(const char* string, char** startOfNotProcessedString);
		long double strtold(const char* string, char** startOfNotProcessedString); // TODO: Implement me!

		int putenv(char* nameAndValue);
		char* getenv(const char* name);
		int setenv(const char* name, const char* value, int overwrite);
		int unsetenv(const char* name);

		void* bsearch(const void* element, const void* array, size_t arrayLength, size_t elementSize,
			int (*compare)(const void*, const void*));
		void qsort(void* array, size_t arrayLength, size_t elementSize, int (*comparator)(const void*, const void*));
		void qsort_r(void* array, size_t arrayLength, size_t elementSize, int (*comparator)(const void*, const void*, void*),
			void* argument);

		#define RAND_MAX 2147483647
		int rand(void);
		void srand(unsigned int seed);
      long int random(void);
      void srandom(unsigned int seed);

		int system(const char* command);

      char* mkdtemp(char* template); // TODO: Implement me!
		int mkstemp(char* template);
		char* mktemp(char* template); // TODO: Implement me!
      int mkstemps(char* template, int suffixlen);	// TODO: Implement me!

      static inline __attribute__((always_inline)) int abs(int value) {
      	if (value < 0) {
      		return -value;
      	} else {
      		return value;
      	}
      }

      static inline __attribute__((always_inline)) long int labs(long int value) {
      	if (value < 0) {
      		return -value;
      	} else {
      		return value;
      	}
      }

      static inline __attribute__((always_inline)) long long int llabs(long long int value) {
      	if (value < 0) {
      		return -value;
      	} else {
      		return value;
      	}
      }

      extern int mbtowc(wchar_t* wideCharacterString, const char* multiByteString,
			size_t count); // TODO: Implement me!
	#endif

#endif
