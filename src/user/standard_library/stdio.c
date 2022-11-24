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
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "user/util/dynamic_array.h"
#include "user/util/dynamic_array_utils.h"
#include "user/util/scanner.h"

#include "util/formatter.h"
#include "util/string_stream_writer.h"
#include "util/string_stream_reader.h"

FILE stdinStream = {
	.fileDescriptorIndex = STDIN_FILENO,
	.flags = O_RDONLY,
	.initialized = false
};

FILE stdoutStream = {
	.fileDescriptorIndex = STDOUT_FILENO,
	.flags = O_WRONLY,
	.initialized = false
};

FILE stderrStream = {
	.fileDescriptorIndex = STDERR_FILENO,
	.flags = O_WRONLY,
	.initialized = false
};

static bool initialized = false;
static struct DoubleLinkedList allStreamsList;

static void initializeIfNot(void) {
	if (!initialized) {
		doubleLinkedListInitialize(&allStreamsList);
		doubleLinkedListInsertAfterLast(&allStreamsList, &stdin->listElement);
		doubleLinkedListInsertAfterLast(&allStreamsList, &stdout->listElement);
		doubleLinkedListInsertAfterLast(&allStreamsList, &stderr->listElement);
		initialized = true;
	}
}

static void initializeStreamIfNot(FILE* stream) {
	if (stream != NULL && !stream->initialized) {
		int bufferMode = _IONBF;
		struct stat statInstance;
		if (fstat(stream->fileDescriptorIndex, &statInstance) == 0) {
			stream->st_mode = statInstance.st_mode;
			if (S_ISREG(statInstance.st_mode) || S_ISFIFO(statInstance.st_mode)) {
				bufferMode = _IOFBF;
			} else if (S_ISCHR(statInstance.st_mode)) {
				bufferMode = _IOLBF;
			}
		}

		stream->bufferMode = bufferMode;
		fileDescriptorStreamReaderInitialize(&stream->fileDescriptorStreamReader, stream->fileDescriptorIndex);
		fileDescriptorStreamWriterInitialize(&stream->fileDescriptorStreamWriter, stream->fileDescriptorIndex);

		if ((bufferMode == _IOFBF || bufferMode == _IOLBF) && stream->fileDescriptorIndex != STDERR_FILENO) {
			bufferedStreamReaderInitialize(&stream->bufferedStreamReader, &stream->fileDescriptorStreamReader.streamReader, stream->buffer, BUFSIZ);
			stream->streamReader = &stream->bufferedStreamReader.streamReader;

			bufferedStreamWriterInitialize(&stream->bufferedStreamWriter, &stream->fileDescriptorStreamWriter.streamWriter, stream->buffer, BUFSIZ, bufferMode == _IOLBF);
			stream->streamWriter = &stream->bufferedStreamWriter.streamWriter;

		} else {
			stream->streamReader = &stream->fileDescriptorStreamReader.streamReader;
			stream->streamWriter = &stream->fileDescriptorStreamWriter.streamWriter;
		}

		stream->initialized = true;
	}
}

static bool isStreamValid(FILE* stream) {
	if (stream != NULL) {
		bool result = doubleLinkedListContainsFoward(&allStreamsList, &stream->listElement);
		assert(result);
		return result;
	} else {
		errno = EINVAL;
		return false;
	}
}

int sscanf(const char* string, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vsscanf(string, format, ap);
	va_end(ap);
	return result;
}

int vsscanf(const char* string, const char* format, va_list ap) {
	int returnedErrorId;
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);
	int result = scannerScanFormat(&stringStreamReader.streamReader, format, ap, NULL,
			(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
			(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize,
			&returnedErrorId);
	if (result == EOF) {
		errno = returnedErrorId;
	}
	return result;
}

int vdprintf(int fileDescriptorIndex, const char* format, va_list ap) {
	struct FileDescriptorStreamWriter fileDescriptorStreamWriter;
	fileDescriptorStreamWriterInitialize(&fileDescriptorStreamWriter, fileDescriptorIndex);

	const int bufferSize = 256;
	char buffer[bufferSize];
	struct BufferedStreamWriter bufferedStreamWriter;
	bufferedStreamWriterInitialize(&bufferedStreamWriter, &fileDescriptorStreamWriter.streamWriter, buffer, bufferSize, false);
	streamWriterVaFormat(&bufferedStreamWriter.streamWriter, format, ap);
	bufferedStreamWriterFlush(&bufferedStreamWriter);

	if (streamWriterError(&bufferedStreamWriter.streamWriter)) {
		return EOF;
	} else {
		return streamWriterGetWrittenCharacterCount(&fileDescriptorStreamWriter.streamWriter);
	}
}

int dprintf(int fileDescriptorIndex, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vdprintf(fileDescriptorIndex, format, ap);
	va_end(ap);
	return result;
}

static int flagsForMode(const char* mode) {
	int flags = 0;
	if (strcmp("r", mode) == 0 || strcmp("rb", mode) == 0) {
		flags = O_RDONLY;

	} else if (strcmp("w", mode) == 0 || strcmp("wb", mode) == 0) {
		flags = O_WRONLY | O_CREAT | O_TRUNC;

	} else if (strcmp("a", mode) == 0 || strcmp("ab", mode) == 0) {
		flags = O_WRONLY | O_CREAT | O_APPEND;

	} else if (strcmp("r+", mode) == 0 || strcmp("rb+", mode) == 0 || strcmp("r+b", mode) == 0) {
		flags = O_RDWR;

	} else if (strcmp("w+", mode) == 0 || strcmp("wb+", mode) == 0 || strcmp("w+b", mode) == 0) {
		flags = O_RDWR | O_CREAT | O_TRUNC;

	} else if (strcmp("a+", mode) == 0 || strcmp("ab+", mode) == 0 || strcmp("wb+", mode) == 0) {
		flags = O_RDWR | O_CREAT | O_APPEND;
	}

	return flags;
}

FILE* fopen(const char* path, const char* mode) {
	initializeIfNot();

	int flags = flagsForMode(mode);
	if (flags != 0) {
		FILE* stream = malloc(sizeof(FILE));
		if (stream != NULL) {
			memset(stream, 0, sizeof(FILE));
			stream->flags = flags;
			stream->fileDescriptorIndex = open(path, flags);
			if (stream->fileDescriptorIndex != -1) {
				doubleLinkedListInsertAfterLast(&allStreamsList, &stream->listElement);
				return stream;
			} else {
				free(stream);
			}
		}

	} else {
		errno = EINVAL;
	}

	return NULL;
}

FILE* fdopen(int fileDescriptorIndex, const char* mode) {
	initializeIfNot();

	int flags = flagsForMode(mode);
	FILE* stream = malloc(sizeof(FILE));
	if (stream != NULL) {
		memset(stream, 0, sizeof(FILE));
		stream->flags = flags;
		stream->fileDescriptorIndex = fileDescriptorIndex;
		if (stream->fileDescriptorIndex != -1) {
			doubleLinkedListInsertAfterLast(&allStreamsList, &stream->listElement);
			return stream;
		} else {
			free(stream);
		}
	}

	return NULL;
}

int fclose(FILE* stream) {
	initializeIfNot();

	if (stream == stdin || stream == stdout || stream == stderr) {
		return fflush(stream);

	} else {
		if (isStreamValid(stream)) {
			int result = fflush(stream);
			doubleLinkedListRemove(&allStreamsList, &stream->listElement);
			close(stream->fileDescriptorIndex);
			free(stream->dynamicAllocatedBuffer);
			free(stream);
			return result;

		} else {
			return EOF;
		}
	}
}

int printf(const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vprintf(format, ap);
	va_end(ap);
	return result;
}

int vprintf(const char* format, va_list ap) {
	return vfprintf(stdout, format, ap);
}

int vfprintf(FILE* stream, const char* format, va_list ap) {
	initializeIfNot();

	int result;
	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		result = streamWriterVaFormat(stream->streamWriter, format, ap);
		if (streamWriterError(stream->streamWriter)) {
			stream->errorId = streamWriterError(stream->streamWriter);
			result = EOF;
		}

	} else {
		result = -1;
	}
	return result;
}

int fprintf(FILE* stream, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vfprintf(stream, format, ap);
	va_end(ap);
	return result;
}

int fputc(int c, FILE* stream) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		ssize_t result = streamWriterWriteCharacter(stream->streamWriter, c);
		if (result == EOF) {
			if (streamWriterError(stream->streamWriter)) {
				stream->errorId = streamWriterError(stream->streamWriter);
				errno = stream->errorId;
			}
			return EOF;
		} else {
			return c;
		}

	} else {
		return EOF;
	}
}

static off_t discardReaderBuffer(FILE* stream, bool ignoreFileType) {
	off_t adjustment = 0;
	if ((ignoreFileType || S_ISREG(stream->st_mode)) && &stream->bufferedStreamReader.streamReader == stream->streamReader) {
		adjustment -= bufferedStreamReaderAvailable(&stream->bufferedStreamReader);
		bufferedStreamReaderDiscardBuffered(&stream->bufferedStreamReader);
	}
	if (stream->streamReader != NULL && streamReaderIsNextCharacterBuffered(stream->streamReader)) {
		int ignored;
		streamReaderReadCharacter(stream->streamReader, &ignored);
		adjustment--;
	}
	return adjustment;
}

int fflush(FILE* stream) {
	initializeIfNot();

	if (stream != NULL) {
		if (isStreamValid(stream) && stream->initialized) {
			/*
          * For input streams associated with seekable files (e.g., disk
          * files, but not pipes or terminals), fflush() discards any
          * buffered data that has been fetched from the underlying file, but
          * has not been consumed by the application.
          *
          * https://man7.org/linux/man-pages/man3/fflush.3.html
			 */
			off_t adjustment = discardReaderBuffer(stream, false);

			if (&stream->bufferedStreamWriter.streamWriter == stream->streamWriter) {
				bufferedStreamWriterFlush(&stream->bufferedStreamWriter);
				if (streamWriterError(stream->streamWriter)) {
					stream->errorId = streamWriterError(stream->streamWriter);
					errno = stream->errorId;
					return EOF;
				}
			}

			if (adjustment != 0) {
				assert(adjustment < 0);
				int result = lseek(stream->fileDescriptorIndex, adjustment, SEEK_CUR);
				if (result == -1) {
					return EOF;
				}
			}
		}

	} else {
		struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&allStreamsList);
		while (listElement != NULL) {
			FILE* stream = (void*) listElement;
			listElement = listElement->next;
			fflush(stream);
		}
	}

	return 0;
}

size_t fwrite(const void* buffer, size_t elementSize, size_t elementCount, FILE* stream) {
	initializeIfNot();

	size_t result;
	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		result = streamWriterWrite(stream->streamWriter, buffer, elementSize * elementCount);
		if (result == EOF) {
			result = 0;
		}
		result /= elementSize;

		if (streamWriterError(stream->streamWriter)) {
			stream->errorId = streamWriterError(stream->streamWriter);
			errno = stream->errorId;

		} else if (!streamWriterMayAcceptMoreData(stream->streamWriter)) {
			stream->reachedEnd = true;
		}

	} else {
		result  = 0;
	}
	return result;
}

char* fgets(char* string, int size, FILE* stream) {
	initializeIfNot();

	if (isStreamValid(stream) && size > 0) {
		initializeStreamIfNot(stream);

		int count = 0;

		while (count + 1 < size) {
			int character;
			ssize_t result = streamReaderReadCharacter(stream->streamReader, &character);

			if (result == EOF) {
				if (streamReaderError(stream->streamReader)) {
					stream->errorId = streamReaderError(stream->streamReader);
					errno = stream->errorId;

				} else {
					stream->reachedEnd = true;
				}

				if (count == 0) {
					return NULL;
				} else {
					break;
				}

			} else {
				string[count++] = character;
				if (character == '\n') {
					break;
				}
			}
		}
		string[count] = '\0';
		return string;

	} else {
		return NULL;
	}
}

int fgetc(FILE* stream) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		int character;
		ssize_t result = streamReaderReadCharacter(stream->streamReader, &character);
		if (result == EOF) {
			if (streamReaderError(stream->streamReader)) {
				stream->errorId = streamReaderError(stream->streamReader);

			} else {
				assert(!streamReaderMayHasMoreDataAvailable(stream->streamReader));
				stream->reachedEnd = true;
			}
			return EOF;

		} else {
			return character;
		}

	} else {
		return EOF;
	}
}

int fscanf(FILE* stream, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int result = vfscanf(stream, format, ap);
	va_end(ap);
	return result;
}

int vfscanf(FILE* stream, const char* format, va_list ap) {
	initializeIfNot();

	int result;
	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		int errorId;
		result = scannerScanFormat(stream->streamReader, format, ap, NULL,
				(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
				(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize,
				&errorId);
		if (errorId != 0) {
			stream->errorId = errorId;
			errno = errorId;

		} else if (streamReaderError(stream->streamReader)) {
			stream->errorId = streamReaderError(stream->streamReader);
			errno = stream->errorId;

		} else if (!streamReaderMayHasMoreDataAvailable(stream->streamReader)) {
			stream->reachedEnd = true;
		}

	} else {
		result = EOF;
	}

	return result;
}

size_t fread(void* buffer, size_t elementSize, size_t elementCount, FILE* stream) {
	initializeIfNot();

	size_t result;
	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		result = streamReaderRead(stream->streamReader, buffer, elementSize * elementCount);
		if (result == EOF) {
			result = 0;
		}
		result /= elementSize;

		if (streamReaderError(stream->streamReader)) {
			stream->errorId = streamReaderError(stream->streamReader);
			errno = stream->errorId;

		} else if (!streamReaderMayHasMoreDataAvailable(stream->streamReader)) {
			stream->reachedEnd = true;
		}

	} else {
		result  = 0;
	}
	return result;
}

int fileno(FILE *stream) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		return stream->fileDescriptorIndex;
	} else {
		errno = EBADF;
		return -1;
	}
}

void closeAllStreams(void) {
	initializeIfNot();

	struct DoubleLinkedListElement* listElement = doubleLinkedListFirst(&allStreamsList);
	while (listElement != NULL) {
		FILE* stream = (void*) listElement;
		listElement = listElement->next;
		fclose(stream);
	}
}

static int writeString(const char* string, FILE* stream, bool appendEndOfLine) {
	initializeIfNot();

	int result = 0;
	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		streamWriterWriteString(stream->streamWriter, string, UINT_MAX);
		if (streamWriterError(stream->streamWriter)) {
			stream->errorId = streamWriterError(stream->streamWriter);
			errno = stream->errorId;
			result = EOF;

		} else if (appendEndOfLine) {
			streamWriterWriteCharacter(stream->streamWriter, '\n');
			if (streamWriterError(stream->streamWriter)) {
				stream->errorId = streamWriterError(stream->streamWriter);
				errno = stream->errorId;
				result = EOF;
			}
		}
	}

	return result;
}

int fputs(const char* string, FILE* stream) {
	return writeString(string, stream, false);
}

int puts(const char* string) {
	return writeString(string, stdout, true);
}

ssize_t getline(char** buffer, size_t* bufferSize, FILE *stream) {
	return getdelim(buffer, bufferSize, '\n', stream);
}

ssize_t getdelim(char** buffer, size_t* bufferSize, int delimiter, FILE* stream) {
	initializeIfNot();

	ssize_t result = EOF;
	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		struct DynamicArrayUtilsMemoryAllocatorContext memoryAllocatorContext;
		memoryAllocatorContext.buffer = *buffer;
		memoryAllocatorContext.bufferSize = *bufferSize;

		struct DynamicArray dynamicArray;
		dynamicArrayInitialize(&dynamicArray, sizeof(char), &memoryAllocatorContext,
				(void (*)(void*, void*)) &dynamicArrayUtilsMemoryAllocatorRelease,
				(void* (*)(void*, void*, size_t)) &dynamicArrayUtilsMemoryAllocatorResize);

		int errorId = 0;
		size_t count = 0;
		while (true) {
			int characterAsInteger;
			if (streamReaderReadCharacter(stream->streamReader, &characterAsInteger) == EOF) {
				if (streamReaderError(stream->streamReader)) {
					errorId = streamReaderError(stream->streamReader);
					stream->errorId = errorId;

				} else {
					assert(!streamReaderMayHasMoreDataAvailable(stream->streamReader));
					stream->reachedEnd = true;
				}
				break;

			} else {
				uint8_t character = characterAsInteger;
				if (dynamicArrayInsertAfterLast(&dynamicArray, &character) == NULL) {
					errorId = ENOMEM;
					break;
				}
				count++;

				if (characterAsInteger == delimiter) {
					break;
				}
			}
		}

		if (count > 0 && errorId == 0) {
			uint8_t character = '\0';
			if (dynamicArrayInsertAfterLast(&dynamicArray, &character) == NULL) {
				result = EOF;
				errorId = ENOMEM;
			} else {
				result = count;
			}
		} else {
			result = EOF;
		}

		*buffer = memoryAllocatorContext.buffer;
		*bufferSize = memoryAllocatorContext.bufferSize;

		if (errorId != 0) {
			errno = errorId;
		}

	}

	return result;
}

void clearerr(FILE* stream) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);
		stream->errorId = 0;
		stream->reachedEnd = 0;
	}
}

int ungetc(int c, FILE* stream) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		return streamReaderUndoReadCharacter(stream->streamReader, c) == 1 ? c : EOF;

	} else {
		return EOF;
	}
}

int setlinebuf(FILE* stream) {
	return setvbuf(stream, NULL, _IOLBF, 0);
}

void setbuf(FILE* stream, char* buffer) {
	setvbuf(stream, buffer, buffer == NULL ? _IONBF : _IOFBF, BUFSIZ);
}

void setbuffer(FILE* stream, char* buffer, size_t bufferSize) {
	setvbuf(stream, buffer, buffer == NULL ? _IONBF : _IOFBF, bufferSize);
}

int setvbuf(FILE* stream, char* buffer, int mode, size_t bufferSize) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		if (mode == _IONBF) {
			stream->streamReader = &stream->fileDescriptorStreamReader.streamReader;
			stream->streamWriter = &stream->fileDescriptorStreamWriter.streamWriter;
			stream->bufferMode = _IOFBF;

		} else if (mode == _IOFBF || mode == _IOLBF) {
			if (buffer == NULL) {
				bufferSize = bufferSize == 0 ? BUFSIZ : bufferSize;
				if (bufferSize > BUFSIZ) {
					buffer = realloc(stream->dynamicAllocatedBuffer, bufferSize);
					if (buffer == NULL) {
						errno = ENOMEM;
						return EOF;
					}

				} else {
					buffer = stream->buffer;
				}
			}

			stream->bufferMode = mode;

			bufferedStreamReaderInitialize(&stream->bufferedStreamReader, &stream->fileDescriptorStreamReader.streamReader, buffer, bufferSize);
			stream->streamReader = &stream->bufferedStreamReader.streamReader;

			bufferedStreamWriterInitialize(&stream->bufferedStreamWriter, &stream->fileDescriptorStreamWriter.streamWriter, buffer, bufferSize, mode == _IOLBF);
			stream->streamWriter = &stream->bufferedStreamWriter.streamWriter;

		} else {
			return EOF;
		}

		return 0;

	} else {
		errno = EINVAL;
		return EOF;
	}
}

void flockfile(FILE* stream) {
}

void funlockfile(FILE* stream) {
}

int getc_unlocked(FILE* stream) {
	return getc(stream);
}

int vasprintf(char** result, const char* format, va_list ap) {
	// TODO: Implement me!
	assert(false);
	return 0;
}

int asprintf(char** result, const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	int resultToReturn = vasprintf(result, format, ap);
	va_end(ap);
	return resultToReturn;
}

int fseek(FILE* stream, long offset, int whence) {
	return fseeko(stream, (long) offset, whence);
}

int fseeko(FILE *stream, off_t offset, int whence) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		off_t adjustment = discardReaderBuffer(stream, true);

		if (stream->streamWriter == (void*) &stream->bufferedStreamWriter) {
			int toBeFlushed = bufferedStreamWriterToBeFlushed(&stream->bufferedStreamWriter);
			adjustment += toBeFlushed;
			ssize_t flushed = bufferedStreamWriterFlush(&stream->bufferedStreamWriter);
			if (flushed == EOF) {
				stream->errorId = streamWriterError(stream->streamWriter);
				errno = stream->errorId;
				return EOF;

			} else {
				assert(flushed == toBeFlushed);
			}
		}

		int result = lseek(stream->fileDescriptorIndex, offset + (whence == SEEK_CUR ? adjustment : 0), whence);
		if (result != -1) {
			stream->reachedEnd = false;
			return 0;
		}
	}

	return EOF;
}

long ftell(FILE* stream) {
	return (long) ftello(stream);
}

off_t ftello(FILE* stream) {
	initializeIfNot();

	if (isStreamValid(stream)) {
		initializeStreamIfNot(stream);

		int whence;
		if ((stream->flags & O_APPEND) && (stream->flags & O_WRONLY)) {
			whence = SEEK_END;
		} else {
			whence = SEEK_CUR;
		}
		off_t result = lseek(stream->fileDescriptorIndex, 0, whence);
		if (result != -1) {
			if (stream->streamReader == (void*) &stream->bufferedStreamReader) {
				int available = bufferedStreamReaderAvailable(&stream->bufferedStreamReader);
				assert(result >= available);
				result -= available;
			}

			if (stream->streamReader != NULL && streamReaderIsNextCharacterBuffered(stream->streamReader)) {
				assert(result > 0);
				result--;
			}

			if (stream->streamWriter == (void*) &stream->bufferedStreamWriter) {
				result += bufferedStreamWriterToBeFlushed(&stream->bufferedStreamWriter);
			}

			return result;

		} else {
			return EOF;
		}


	} else {
		errno = EINVAL;
		return EOF;
	}
}

int remove(const char* path) {
	struct stat statInstance;

	if (stat(path, &statInstance) == 0) {
		if (S_ISDIR(statInstance.st_mode)) {
			return rmdir(path);
		} else {
			return unlink(path);
		}
	}

	return -1;
}

FILE* tmpfile(void) {
	assert(false); // TODO: Implement me!
	return NULL;
}

FILE* popen(const char* command, const char* type) {
	assert(false); // TODO: Implement me!
	return NULL;
}

int pclose(FILE* stream) {
	assert(false); // TODO: Implement me!
	return 0;
}
