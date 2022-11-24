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

#include "util/ring_buffer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void testCopy(void) {
	struct RingBuffer ringBuffer;
	char internalBuffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
	char buffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '\0'};
	ringBufferInitialize(&ringBuffer, internalBuffer, sizeof(internalBuffer));

	ringBuffer.begin = 7;

	assert(ringBufferIsEmpty(&ringBuffer));
	assert(ringBufferSize(&ringBuffer) == 0);
	int copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 0);
	copyCount = ringBufferCopy(&ringBuffer, buffer, 0, 0);
	assert(copyCount == 0);
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 100);
	assert(copyCount == 0);

	const char* string = "abc";
	ringBufferWrite(&ringBuffer, string, strlen(string));
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 3);
	buffer[copyCount] = '\0';
	assert(strcmp("abc", buffer) == 0);

	string = "defgh";
	ringBufferWrite(&ringBuffer, string, 4);
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 7);
	buffer[copyCount] = '\0';
	assert(strcmp("abcdefg", buffer) == 0);

	string = "h";
	ringBufferWrite(&ringBuffer, string, strlen(string));
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("abcdefgh", buffer) == 0);

	string = "12345";
	ringBufferWrite(&ringBuffer, string, strlen(string));
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("fgh12345", buffer) == 0);

	string = "67890";
	ringBufferWrite(&ringBuffer, string, strlen(string));
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("34567890", buffer) == 0);

	string = "		 abcdefghijklmnopqrstuvwxyz";
	ringBufferWrite(&ringBuffer, string, strlen(string));
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("stuvwxyz", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 4);
	assert(copyCount == 4);
	buffer[copyCount] = '\0';
	assert(strcmp("wxyz", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, -3);
	assert(copyCount == 3);
	buffer[copyCount] = '\0';
	assert(strcmp("xyz", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, -8);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("stuvwxyz", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 9);
	assert(copyCount == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 100);
	assert(copyCount == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, -1);
	assert(copyCount == 1);
	buffer[copyCount] = '\0';
	assert(strcmp("z", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, -2);
	assert(copyCount == 2);
	buffer[copyCount] = '\0';
	assert(strcmp("yz", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, -4);
	assert(copyCount == 4);
	buffer[copyCount] = '\0';
	assert(strcmp("wxyz", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, -100);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("stuvwxyz", buffer) == 0);

	ringBufferWrite(&ringBuffer, "1", 1);
	ringBufferWrite(&ringBuffer, "2", 1);
	ringBufferWrite(&ringBuffer, "3", 1);
	ringBufferWrite(&ringBuffer, "4", 1);
	ringBufferWrite(&ringBuffer, "5", 1);
	ringBufferWrite(&ringBuffer, "6", 1);
	ringBufferWrite(&ringBuffer, "7", 1);
	ringBufferWrite(&ringBuffer, "8", 1);
	copyCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer) - 1, 0);
	assert(copyCount == 8);
	buffer[copyCount] = '\0';
	assert(strcmp("12345678", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, 1, 0);
	assert(copyCount == 1);
	buffer[copyCount] = '\0';
	assert(strcmp("1", buffer) == 0);

	copyCount = ringBufferCopy(&ringBuffer, buffer, 4, 0);
	assert(copyCount == 4);
	buffer[copyCount] = '\0';
	assert(strcmp("1234", buffer) == 0);
}

static void testRead(void) {
	struct RingBuffer ringBuffer;
	char internalBuffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
	ringBufferInitialize(&ringBuffer, internalBuffer, sizeof(internalBuffer));

	char buffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
	ringBuffer.begin = 5;
	ringBufferWrite(&ringBuffer, "abc", 3);
	ringBufferWrite(&ringBuffer, "def", 3);
	ringBufferWrite(&ringBuffer, "gh", 2);
	int readCount = ringBufferRead(&ringBuffer, buffer, 4);
	assert(readCount == 4);
	buffer[readCount] = '\0';
	assert(strcmp("abcd", buffer) == 0);
	readCount = ringBufferRead(&ringBuffer, buffer, 4);
	assert(readCount == 4);
	buffer[readCount] = '\0';
	assert(strcmp("efgh", buffer) == 0);
	assert(ringBufferIsEmpty(&ringBuffer));

	ringBufferWrite(&ringBuffer, "123456", 6);
	readCount = ringBufferRead(&ringBuffer, buffer, 1);
	buffer[readCount] = '\0';
	assert(strcmp("1", buffer) == 0);
	assert(ringBufferSize(&ringBuffer) == 5);
	readCount = ringBufferRead(&ringBuffer, buffer, sizeof(buffer));
	buffer[readCount] = '\0';
	assert(strcmp("23456", buffer) == 0);
	assert(ringBufferIsEmpty(&ringBuffer));

	ringBufferWrite(&ringBuffer, "W", 1);
	ringBufferWrite(&ringBuffer, "X", 1);
	ringBufferWrite(&ringBuffer, "Y", 1);
	ringBufferWrite(&ringBuffer, "Z", 1);
	readCount = ringBufferRead(&ringBuffer, buffer, 4);
	buffer[readCount] = '\0';
	assert(strcmp("WXYZ", buffer) == 0);
	assert(ringBufferIsEmpty(&ringBuffer));
}

static void testDiscard(void) {
	struct RingBuffer ringBuffer;
	char internalBuffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
	ringBufferInitialize(&ringBuffer, internalBuffer, sizeof(internalBuffer));

	char buffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
	ringBuffer.begin = 6;
	ringBufferWrite(&ringBuffer, "abc", 3);
	ringBufferWrite(&ringBuffer, "def", 3);
	ringBufferWrite(&ringBuffer, "gh", 2);

	ringBufferDiscard(&ringBuffer, 4);

	int readCount = ringBufferRead(&ringBuffer, buffer, 4);
	assert(readCount == 4);
	buffer[readCount] = '\0';
	assert(strcmp("efgh", buffer) == 0);
	assert(ringBufferIsEmpty(&ringBuffer));

	ringBuffer.begin = 7;
	ringBufferWrite(&ringBuffer, "123", 3);
	ringBufferWrite(&ringBuffer, "456", 3);
	ringBufferWrite(&ringBuffer, "78", 2);
	ringBufferDiscard(&ringBuffer, -4);

	readCount = ringBufferRead(&ringBuffer, buffer, 4);
	assert(readCount == 4);
	buffer[readCount] = '\0';
	assert(strcmp("1234", buffer) == 0);
	assert(ringBufferSize(&ringBuffer) == 0);

	ringBuffer.begin = 7;
	ringBufferWrite(&ringBuffer, "123", 3);
	ringBufferWrite(&ringBuffer, "45", 2);
	ringBufferWrite(&ringBuffer, "67", 2);
	ringBufferWrite(&ringBuffer, "89", 2);
	assert(ringBufferSize(&ringBuffer) == 9);
	ringBufferDiscard(&ringBuffer, 3);
	assert(ringBufferSize(&ringBuffer) == 6);
	ringBufferDiscard(&ringBuffer, -3);
	assert(ringBufferSize(&ringBuffer) == 3);

	readCount = ringBufferRead(&ringBuffer, buffer, 3);
	assert(readCount == 3);
	buffer[readCount] = '\0';

	assert(strcmp("456", buffer) == 0);
}

static void testOverWrite(void) {
	struct RingBuffer ringBuffer;
	char internalBuffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
	ringBufferInitialize(&ringBuffer, internalBuffer, sizeof(internalBuffer));

	char buffer[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};

	ringBuffer.begin = 6;
	ringBufferWrite(&ringBuffer, "abcdef", 6);

	assert(ringBufferSize(&ringBuffer) == 6);
	int readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("abcdef", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "ABC", 3, -6);
	assert(ringBufferSize(&ringBuffer) == 6);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("ABCdef", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "12345", 5, -6);
	assert(ringBufferSize(&ringBuffer) == 6);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("12345f", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "abcdef", 6, -5);
	assert(ringBufferSize(&ringBuffer) == 7);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("1abcdef", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "23XXX", 2, 0);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("1abcdef23", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "ABCDEFGHIJKLMNOP", 9, -100);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("ABCDEFGHI", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "abcdefghijkl", 12, -100);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("defghijkl", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "ABC", 3, -6);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("defABCjkl", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "ABC", 3, -15);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("ABCABCjkl", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "123456", 6, -2);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("BCj123456", buffer) == 0);

	ringBufferOverWrite(&ringBuffer, "abcd", 4, -1);
	assert(ringBufferSize(&ringBuffer) == 9);
	readCount = ringBufferCopy(&ringBuffer, buffer, sizeof(buffer), 0);
	buffer[readCount] = '\0';
	assert(strcmp("12345abcd", buffer) == 0);
}

int main(int argc, char** argv) {
	testCopy();
	testRead();
	testDiscard();
	testOverWrite();

	return 0;
}
