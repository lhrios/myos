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
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "util/math_utils.h"
#include "util/string_stream_reader.h"

#include "user/util/dynamic_array.h"
#include "user/util/scanner.h"

static int parseFloat(struct StreamReader* streamReader, bool allowSpaceBefore, const char** startOfNotProcessedString, long double* output) {
	bool conversionPerformed = false;
	long double localOutput = 0;
	int result = 0;

	size_t consumedCharacterCount = 0;
	int character;
	bool stop = streamReaderPeekCharacter(streamReader, &character) == EOF;

	if (allowSpaceBefore) {
		while (!stop && isspace(character)) {
			consumedCharacterCount++;
			stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
		}
	}

	if (!stop) {
		bool positive = true;
		if (character == '+') {
			consumedCharacterCount++;
			stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
		} else if (character == '-') {
			consumedCharacterCount++;
			stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
			positive = false;
		}

		if (!stop) {
			if (!isdigit(character)) {
				size_t bufferSize = strlen("infinity");
				char buffer[bufferSize];
				size_t length = 0;

				while (length < bufferSize) {
					if (!stop) {
						buffer[length++] = tolower(character);
					}

					if (stop || length == bufferSize) {
						if (length == 8 && strncmp("infinity", buffer, length) == 0) {
							if (positive) {
								localOutput = INFINITY;
							} else {
								localOutput = -INFINITY;
							}
							conversionPerformed = true;
							consumedCharacterCount += 8;

						} else if (length >= 3 && strncmp("inf", buffer, 3) == 0) {
							if (positive) {
								localOutput = INFINITY;
							} else {
								localOutput = -INFINITY;
							}
							conversionPerformed = true;
							consumedCharacterCount += 3;

						} else if (length >= 3 && strncmp("nan", buffer, 3) == 0) {
							if (positive) {
								localOutput = NAN;
							} else {
								localOutput = -NAN;
							}
							conversionPerformed = true;
							consumedCharacterCount += 3;
						}

						break;
					}
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
				}

			} else {
				bool hexadecimal = false;

				if (character == '0') {
					consumedCharacterCount++;
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;

					if (!stop) {
						if (character == 'x' || character == 'X') {
							hexadecimal = true;
							consumedCharacterCount++;
							stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
						}

					}
				}

				int digitsCount = 0;
				int remainderDigitsCount = 0;
 				int exponentDigitsCount = 0;

 				bool isZero = true;
				bool isExponentNegative = false;
				bool foundPeriod = false;
				bool foundExponentCharacter = false;
				bool exponentOverflowOrUnderflow = false;
				long double remainder = 0;
				long double exponent = 0;

				while (!stop) {
					if (character == '.') {
						if (foundPeriod) {
							break;
						} else {
							if (foundExponentCharacter) {
								break;
							} else {
								foundPeriod = true;
							}
						}

					} else if ((hexadecimal && (character == 'P' || character == 'p')) || (!hexadecimal && (character == 'E' || character == 'e'))) {
						if (foundExponentCharacter) {
							break;

						} else {
							stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
							if (!stop) {
								if (character == '-' || character == '+') {
									isExponentNegative = character == '-';
									stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
									if (!stop && isdigit(character)) {
										consumedCharacterCount += 2;
										foundExponentCharacter = true;
										continue;

									} else {
										break;
									}

								} else if (isdigit(character)) {
									consumedCharacterCount++;
									foundExponentCharacter = true;
									continue;

								} else {
									break;
								}

							} else {
								break;
							}

							foundExponentCharacter = true;
						}

					} else if (isxdigit(character)) {
						int base = (hexadecimal ? 16 : 10);

						if (foundExponentCharacter) {
							if (!isdigit(character)) {
								break;

							} else {
								if (LDBL_MIN_EXP < exponent && exponent < LDBL_MAX_EXP) {
									if (exponent != 0) {
										exponent *= 10;
									}
									int characterValue = scannerGetCharacterValue(base, character);
									exponent += characterValue;
									exponentDigitsCount++;


								} else {
									exponentOverflowOrUnderflow = true;
								}
							}

						} else if (foundPeriod) {
							if (!hexadecimal && !isdigit(character)) {
								break;

							} else {
								int characterValue = scannerGetCharacterValue(base, character);
								isZero = isZero && character == 0;
								remainder *= base;
								remainder += characterValue;
								remainderDigitsCount++;
							}

						} else {
							if (!hexadecimal && !isdigit(character)) {
								break;

							} else {
								if (digitsCount > 0) {
									localOutput *= base;
								}
								int characterValue = scannerGetCharacterValue(base, character);
								isZero = isZero && character == 0;
								localOutput += characterValue;
							}
						}

						digitsCount++;

					} else {
						break;
					}

					consumedCharacterCount++;
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &character) == EOF;
				}

				if (digitsCount > 0) {
					conversionPerformed = true;

					if (exponentOverflowOrUnderflow) {
						result = ERANGE;
						if (isExponentNegative) {
							localOutput = 0;
						} else {
							localOutput = INFINITY;
						}

					} else {
						if (remainderDigitsCount > 0) {
							localOutput += remainder * mathUtilsPow(hexadecimal ? 16 : 10, -remainderDigitsCount, NULL);
						}

						if (exponentDigitsCount > 0) {
							localOutput *= mathUtilsPow(hexadecimal ? 2 : 10, (isExponentNegative ? -1 : 1) * exponent, NULL);
						}

						if (isinf(localOutput)) {
							result = ERANGE;
						} else {

						}
					}
				}
			}
		}
	}

	if (conversionPerformed) {
		if (startOfNotProcessedString != NULL) {
			struct StringStreamReader* stringStreamReader = (void*) streamReader;
			*startOfNotProcessedString = stringStreamReaderGetString(stringStreamReader) + consumedCharacterCount;
		}
	} else {
		localOutput = 0;
		if (startOfNotProcessedString != NULL) {
			struct StringStreamReader* stringStreamReader = (void*) streamReader;
			*startOfNotProcessedString = stringStreamReaderGetString(stringStreamReader);
		}
	}

	*output = localOutput;

	return result;
}

int scannerParseFloat(const char* string, bool allowSpaceBefore, const char** startOfNotProcessedString, long double* output) {
	struct StringStreamReader stringStreamReader;
	stringStreamReaderInitialize(&stringStreamReader, string, UINT_MAX);

	int result = parseFloat(&stringStreamReader.streamReader, allowSpaceBefore, startOfNotProcessedString, output);
	return result;
}

static const char SPECIFIER_CHARACTERS_STRING[] = "usdixXcf%";
static bool isSpecifierCharacter(char c) {
	return strchr(SPECIFIER_CHARACTERS_STRING, c) != NULL;
}

static bool isContentLengthModifier(char c) {
	return c == 'l' || c == 'L';
}

int scannerScanFormat(struct StreamReader* streamReader, const char *format, va_list ap,
		void* memoryAllocatorContext, void (*memoryAllocatorRelease)(void*, void*), void* (*memoryAllocatorResize)(void*, void*, size_t),
		int* errorId) {
	bool foundPercentCharacter = false;
	bool assignmentAllocation = false;
	int numberOfSuccessfulMatches = 0;
	int contentLengthInBytes = -1;

	char character = '\0';
	int inputCharacter;
	bool stop = streamReaderPeekCharacter(streamReader, &inputCharacter) == EOF;

	*errorId = 0;

	while ((character = *format) && !stop) {
		if (foundPercentCharacter) {
			if (isContentLengthModifier(character)) {
				switch (character) {
					case 'l':
						if (contentLengthInBytes == -1) {
							contentLengthInBytes = 0;
						}
						contentLengthInBytes += 4;
					break;

					case 'L':
						contentLengthInBytes = 8;
					break;
				}

			} else if (isSpecifierCharacter(character)) {
				switch (character) {
					case 's':
						{
							if (assignmentAllocation) {
								struct DynamicArray dynamicArray;
								dynamicArrayInitialize(&dynamicArray, sizeof(char), NULL, memoryAllocatorRelease, memoryAllocatorResize);

								char** string = va_arg(ap, char**);
								while (!stop && isspace(inputCharacter)) {
									stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
								}

								while (!stop && !isspace(inputCharacter)) {
									if (!dynamicArrayInsertAfterLast(&dynamicArray, &inputCharacter)) {
										dynamicArrayClear(&dynamicArray, true);
										stop = true;
										*errorId = ENOMEM;
										break;
									}
									stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
								}
								if (dynamicArraySize(&dynamicArray) > 0) {
									dynamicArrayInsertAfterLast(&dynamicArray, "\0");
									*string = dynamicArrayGetArray(&dynamicArray);
									numberOfSuccessfulMatches++;
								} else {
									*string = NULL;
								}

							} else {
								char* string = va_arg(ap, char*);

								size_t stringLength = 0;
								while (!stop && isspace(inputCharacter)) {
									stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
								}

								while (!stop && !isspace(inputCharacter)) {
									*string = inputCharacter;
									string++;
									stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
									stringLength++;
								}
								if (stringLength > 0) {
									*string = '\0';
									numberOfSuccessfulMatches++;
								}
							}
						}
						break;

					case 'u':
						{
							// TODO: Consider 64 bits integers

							int consumedCharacterCountBefore = streamReaderGetConsumedCharacterCount(streamReader);
							uint32_t* output = va_arg(ap, uint32_t*);
							union ScannerInteger32 integer32;
							int result;
							if ((result = scannerParseInteger32(streamReader, 10, false, true, false, NULL, &integer32)) != 0) {
								stop = true;
								*errorId = ERANGE;

							} else {
								stop = streamReaderPeekCharacter(streamReader, &inputCharacter) == EOF;
								if (consumedCharacterCountBefore < streamReaderGetConsumedCharacterCount(streamReader)) {
									numberOfSuccessfulMatches++;
									*output = integer32.unsignedValue;
								}
							}
						}
						break;

					case 'f':
						{
							int consumedCharacterCountBefore = streamReaderGetConsumedCharacterCount(streamReader);
							long double localOutput;
							int result;
							if ((result = parseFloat(streamReader, true, NULL, &localOutput))) {
								stop = true;
								*errorId = result;
							} else {
								stop = streamReaderPeekCharacter(streamReader, &inputCharacter) == EOF;
								if (consumedCharacterCountBefore < streamReaderGetConsumedCharacterCount(streamReader)) {
									numberOfSuccessfulMatches++;
									if (contentLengthInBytes == -1) {
										float* output = va_arg(ap, float*);
										*output = (float) localOutput;
									} else if (contentLengthInBytes == 4) {
										double* output = va_arg(ap, double*);
										*output = (double) localOutput;
									} else {
										long double* output = va_arg(ap, long double*);
										*output = localOutput;
									}
								}
							}
						}
						break;

					case 'i':
					case 'd':
					case 'x':
					case 'X':
						{
							// TODO: Consider 64 bits integers

							int base;
							if (character == 'i') {
								base = 0;
							} else if (character == 'x' || character == 'X') {
								base = 16;
							} else if (character == 'd') {
								base = 10;
							} else {
								base = 10;
								assert(false);
							}

							int consumedCharacterCountBefore = streamReaderGetConsumedCharacterCount(streamReader);
							int32_t* output = va_arg(ap, int32_t*);
							union ScannerInteger32 integer32;
							int result;
							if ((result = scannerParseInteger32(streamReader, base, true, true, true, NULL, &integer32)) != 0) {
								stop = true;
								*errorId = ERANGE;

							} else {
								stop = streamReaderPeekCharacter(streamReader, &inputCharacter) == EOF;
								if (consumedCharacterCountBefore < streamReaderGetConsumedCharacterCount(streamReader)) {
									numberOfSuccessfulMatches++;
									*output = integer32.signedValue;
								}
							}
						}
						break;

					case 'c':
						{
							char* output = va_arg(ap, char*);
							*output = inputCharacter;
							stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
							numberOfSuccessfulMatches++;
						}
						break;

					case '%':
						{
							if (inputCharacter == '%') {
								stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
							} else {
								stop = true;
							}
						}
						break;

					default:
						assert(false);
						break;
				}

				foundPercentCharacter = false;

			} else if (character == 'm') {
				assignmentAllocation = true;
			}

		} else {
			if (isspace(character)) {
				while (!stop && isspace(inputCharacter)) {
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
				}

			} else if (character == '%') {
				foundPercentCharacter = true;
				assignmentAllocation = false;

			} else {
				if (inputCharacter == character) {
					stop = streamReaderDiscardCurrentAndPeek(streamReader, &inputCharacter) == EOF;
				} else {
					stop = true;
				}
			}
		}

		format++;
	}

	if (!streamReaderMayHasMoreDataAvailable(streamReader)) {
		if (numberOfSuccessfulMatches != 0) {
			return numberOfSuccessfulMatches;
		} else {
			return EOF;
		}

	} else if (*errorId != 0) {
		return EOF;

	} else {
		return numberOfSuccessfulMatches;
	}
}
