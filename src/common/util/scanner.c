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
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/math_utils.h"
#include "util/scanner.h"
#include "util/string_stream_reader.h"

int scannerGetCharacterValue(int base, char character) {
	int value;
	if (isdigit(character)) {
		value = character - '0';

	} else if ('A' <= character && character <= 'Z') {
		value = character - 'A' + 10;

	} else if ('a' <= character && character <= 'z') {
		value = character - 'a' + 10;

	} else {
		value = -1;
	}
	return value;
}

bool scannerIsBaseCharacter(int base, char character) {
	int value = scannerGetCharacterValue(base, character);
	if (value != -1) {
		return value < base;
	} else {
		return false;
	}
}

int scannerParseInteger32(struct StreamReader* streamReader, int base, bool isSigned, bool allowSpaceBefore, bool allowSign, const char** startOfNotProcessedString, union ScannerInteger32* integer32) {
	int bufferSize = 64;
	char buffer[bufferSize];
	int length = 0;
	int character;

	bool stop = streamReaderPeekCharacter(streamReader, &character) == EOF;

	if (isSigned) {
		integer32->signedValue = 0;
	} else {
		integer32->unsignedValue = 0;
	}

	if (allowSpaceBefore) {
		while (!stop && isspace(character)) {
			stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
		}
	}

	bool isNegative = false;
	bool valid = true;
	if (!stop) {
		if (base == 0) {
			if (!scannerIsBaseCharacter(10, character)) {

				if (allowSign) {
					if (character == '-') {
						allowSign = false;
						isNegative = true;
						stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;

					} else if (character == '+') {
						allowSign = false;
						stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;

					} else {
						valid = false;
					}

				} else {
					valid = false;
				}
			}

			if (!stop) {
				if (character == '0') {
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
					if (!stop && (character== 'x' || character == 'X')) {
						base = 16;
						stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
					} else {
						base = 8;
					}

				} else {
					base = 10;
				}
			}

		} else if (base == 16) {
			if (character == '0') {
				stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
				if (!stop && (character == 'x' || character == 'X')) {
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
				}
			}
		}
	}

	int error = 0;
	if (!stop && valid) {
		assert(base != 0);
		bool underflowOrOverflow = false;
		while (!stop) {
			if (length < bufferSize) {
				if (!scannerIsBaseCharacter(base, character)) {
					if (length == 0) {
						if (allowSign) {
							if (character == '-') {
								isNegative = true;

							} else if (character != '+') {
								break;
							}

						} else {
							break;
						}

					} else {
						break;
					}

				} else {
					if (!(length == 1 && buffer[0] == '0' && character == '0')) {
						buffer[length++] = character;
					}
				}
				stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;

			} else {
				underflowOrOverflow = true;
			}
		}

		int64_t result = 0;
		if (!underflowOrOverflow) {
			int64_t multipler = isNegative ? -1 : 1;
			for (int i = length - 1; i >= 0; i--) {
				if ((isSigned && (result < INT_MIN || result > INT_MAX || multipler < INT_MIN || multipler > INT_MAX))
						|| (!isSigned && (result > UINT_MAX || multipler > UINT_MAX))) {
					underflowOrOverflow = true;
					break;

				} else {
					int value = scannerGetCharacterValue(base, buffer[i]);
					if (isSigned) {
						result += value * ((int32_t) multipler);
					} else {
						result += value * ((uint32_t) multipler);
					}
					multipler *= base;
				}
			}
		}

		if (!underflowOrOverflow) {
			if (isSigned) {
				integer32->signedValue = (int32_t) result;
			} else {
				integer32->unsignedValue = (uint32_t) result;
			}

		} else {
			if (isNegative) {
				assert(isSigned && allowSign);
				integer32->signedValue = (int32_t) INT_MIN;
			} else {
				if (isSigned) {
					integer32->signedValue = (int32_t) INT_MAX;
				} else {
					integer32->unsignedValue = (uint32_t) UINT_MAX;
				}
			}
			error =  ERANGE;
		}
	}

	if (startOfNotProcessedString != NULL) {
		struct StringStreamReader* stringStreamReader = (void*) streamReader;
		if (length > 0) {
			*startOfNotProcessedString = stringStreamReaderGetAvailableString(stringStreamReader);
		} else {
			*startOfNotProcessedString = stringStreamReaderGetString(stringStreamReader);
		}
	}

	return error;
}

int scannerParseInt32(const char* string, int base, bool allowSpaceBefore, bool allowSign, const char** startOfNotProcessedString, int32_t* output) {
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	union ScannerInteger32 integer32;
	int result = scannerParseInteger32(&stringStreamReader.streamReader, base, true, allowSpaceBefore, allowSign, startOfNotProcessedString, &integer32);
	*output = integer32.signedValue;

	return result;
}

int scannerParseUint32(const char* string, int base, bool allowSpaceBefore, const char** startOfNotProcessedString, uint32_t* output) {
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	union ScannerInteger32 integer32;
	int result = scannerParseInteger32(&stringStreamReader.streamReader, base, false, allowSpaceBefore, false, startOfNotProcessedString, &integer32);
	*output = integer32.unsignedValue;
	return result;
}
