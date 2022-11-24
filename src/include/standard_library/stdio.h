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

#ifndef STDIO_H
	#define STDIO_H

	#include <stdarg.h>
	#include <stdlib.h>

	#include "standard_library_implementation/file_descriptor_offset_reposition_constants.h"

	#define EOF -1

	#define P_tmpdir "/tmp"

	int sprintf(char* buffer, const char* format, ...);
	int snprintf(char* buffer, size_t size, const char* format, ...);

	int vsprintf(char* buffer, const char* format, va_list ap);
	int vsnprintf(char* buffer, size_t size, const char* format, va_list ap);

	int sscanf(const char* string, const char* format, ...);
	int vsscanf(const char* string, const char* format, va_list ap);

	#ifndef KERNEL_CODE
		#include <stdbool.h>
		#include <stdint.h>

		#include "user/util/buffered_stream_reader.h"
		#include "user/util/buffered_stream_writer.h"
		#include "user/util/file_descriptor_stream_reader.h"
		#include "user/util/file_descriptor_stream_writer.h"

		#include "util/double_linked_list.h"
		#include "util/stream_reader.h"
		#include "util/stream_writer.h"

		#define BUFSIZ 1024

		typedef struct {
			struct DoubleLinkedListElement listElement;

			int fileDescriptorIndex;
			int flags;
			int bufferMode;
			char buffer[BUFSIZ];
			char* dynamicAllocatedBuffer;

			struct StreamReader* streamReader;
			struct BufferedStreamReader bufferedStreamReader;
			struct FileDescriptorStreamReader fileDescriptorStreamReader;

			struct StreamWriter* streamWriter;
			struct BufferedStreamWriter bufferedStreamWriter;
			struct FileDescriptorStreamWriter fileDescriptorStreamWriter;

			bool initialized;
			bool reachedEnd;
			int errorId;
			mode_t st_mode;
		} FILE;

		extern FILE stdinStream;
		extern FILE stdoutStream;
		extern FILE stderrStream;

		#define stdin (&stdinStream)
		#define stdout (&stdoutStream)
		#define stderr (&stderrStream)

		#define _IONBF 0 /* No buffering */
		#define _IOFBF 1 /* Full buffering */
		#define _IOLBF 2 /* Line buffering */

      FILE* fopen(const char* path, const char* mode);
      FILE* fdopen(int fileDescriptorIndex, const char* mode);
      int fclose(FILE* stream);

		int fgetc(FILE* stream);
		#define getchar() getc(stdin)
		#define getc(stream) fgetc(stream)

		int vfprintf(FILE* stream, const char* format, va_list ap);
      int fprintf(FILE* stream, const char* format, ...);
		int vprintf(const char* format, va_list ap);
		int printf(const char* format, ...);

		int vdprintf(int fileDescriptorIndex, const char* format, va_list ap);
		int dprintf(int fileDescriptorIndex, const char* format, ...);

		int fscanf(FILE* stream, const char* format, ... );
		int vfscanf(FILE* stream, const char* format, va_list ap);
		#define scanf(FORMAT, ...) \
			fscanf(stdin, FORMAT, ##__VA_ARGS__)

		static inline __attribute__((always_inline)) int feof(FILE* stream) {
      	return stream->reachedEnd;
      }

		static inline __attribute__((always_inline)) int ferror(FILE* stream) {
      	return stream->errorId;
      }

		int fileno(FILE* stream);

		size_t fread(void* buffer, size_t elementSize, size_t elementCount, FILE* stream);
		size_t fwrite(const void* buffer, size_t elementSize, size_t elementCount, FILE* stream);

		int fputc(int character, FILE* stream);
		#define putc(character, stream) fputc(character, stream)
		#define putchar(character) fputc(character, stdout)

		int ungetc(int character, FILE* stream);

      int fputs(const char* string, FILE* stream);
      int puts(const char* string);

      int fflush(FILE* stream);

      char* fgets(char* string, int size, FILE* stream);

      ssize_t getline(char** buffer, size_t* bufferSize, FILE* stream);
      ssize_t getdelim(char** buffer, size_t* bufferSize, int delimiter, FILE* stream);

		void perror(const char* string);

      void clearerr(FILE* stream);

      int setvbuf(FILE* stream, char* buffer, int mode, size_t bufferSize);
      int setlinebuf(FILE* stream);
      void setbuf(FILE* stream, char* buffer);
      void setbuffer(FILE* stream, char* buffer, size_t bufferSize);

      off_t ftello(FILE* stream);
      long ftell(FILE* stream);

      int fseeko(FILE *stream, off_t offset, int whence);
      int fseek(FILE* stream, long offset, int whence);
		#define rewind(stream) fseek(stream, 0, SEEK_SET);

      int remove(const char* path);
      int rename(const char* oldPath, const char* newPath);

      FILE* tmpfile(void);

      char* ctermid(char* path); // TODO: Implement me!

	#endif

#endif
