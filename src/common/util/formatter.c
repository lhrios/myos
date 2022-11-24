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

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util/formatter.h"
#include "util/math_utils.h"
#include "util/scanner.h"

struct CommonPrintingConfiguration {
	int precision;
	int fieldWidth;
	bool fillWithZeros;
	bool emmitNegativeSign;
	bool isUpperCase;
};

struct IntegerPrintingConfiguration {
	bool isHexadecimal;
	bool isSigned;
};

struct FormatterFormatContext {
	struct StreamWriter* streamWriter;
	bool eof;
	size_t requiredLength;
	struct CommonPrintingConfiguration commonPrintingConfiguration;
	struct IntegerPrintingConfiguration integerPrintingConfiguration;
};

static void emmitCharacter(struct FormatterFormatContext* formatterFormatContext, int character) {
	struct StreamWriter* streamWriter = formatterFormatContext->streamWriter;
	if (!formatterFormatContext->eof) {
		formatterFormatContext->eof = streamWriterWriteCharacter(streamWriter, character) == EOF;
	}
	formatterFormatContext->requiredLength++;
}

static void emmitString(struct FormatterFormatContext* formatterFormatContext, const char* string) {
	while (*string) {
		emmitCharacter(formatterFormatContext, *string);
		string++;
	}
}

static void emmitNegativeSign(struct FormatterFormatContext* formatterFormatContext) {
	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext->commonPrintingConfiguration;

	emmitCharacter(formatterFormatContext, '-');
	commonPrintingConfiguration->emmitNegativeSign = false;
}

static void emmitFloatPadding(struct FormatterFormatContext* formatterFormatContext, int contentLength) {
	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext->commonPrintingConfiguration;

	if (commonPrintingConfiguration->fieldWidth > 0 && commonPrintingConfiguration->fieldWidth > contentLength) {
		for (int i = 0; i < commonPrintingConfiguration->fieldWidth - contentLength; i++) {
			 emmitCharacter(formatterFormatContext,  commonPrintingConfiguration->fillWithZeros ? '0' : ' ');
		}
	}
}

static void emmitFloat(struct FormatterFormatContext* formatterFormatContext, long double number) {
	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext->commonPrintingConfiguration;

	if (signbit(number)) {
		number = -number;
		commonPrintingConfiguration->emmitNegativeSign = true;
	}

	if (isnan(number) || isinf(number)) {
		commonPrintingConfiguration->fillWithZeros = false;
		const char* content;
		if (commonPrintingConfiguration->isUpperCase) {
			content = isnan(number) ? "NAN" : "INF";
		} else {
			content = isnan(number) ? "nan" : "inf";
		}
		emmitFloatPadding(formatterFormatContext, 3 + (commonPrintingConfiguration->emmitNegativeSign ? 1 : 0));
		if (commonPrintingConfiguration->emmitNegativeSign) {
			emmitNegativeSign(formatterFormatContext);
		}
		emmitString(formatterFormatContext, content);

	} else {
		int exponent;
		long double normalized;
		if (number == 0) {
			exponent = 0;
			normalized = 0;
		} else {
			exponent = (int) mathUtilsLog10(number, NULL);
			normalized = number / mathUtilsPow(10, exponent, NULL);
		}

		int precision = commonPrintingConfiguration->precision == -1 ? 6 : commonPrintingConfiguration->precision;

		int contentLength = exponent + 1 + 1 + precision + (commonPrintingConfiguration->emmitNegativeSign ? 1 : 0);
		emmitFloatPadding(formatterFormatContext, contentLength);

		if (commonPrintingConfiguration->emmitNegativeSign) {
			emmitNegativeSign(formatterFormatContext);
		}

		for (int i = 0; i < exponent + 1 + precision; i++) {
			 long double remainder = mathUtilsFmod(normalized, 1, NULL);
			 long double digit = normalized - remainder;

			 if (i == exponent + 1 && precision > 0) {
				 emmitCharacter(formatterFormatContext, '.');
			 }
			 emmitCharacter(formatterFormatContext, '0' + (int) digit);
			 normalized = remainder * 10.L;
		}
	}
}

static void emmit32BitInteger(struct FormatterFormatContext* formatterFormatContext, uint32_t integer) {
	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext->commonPrintingConfiguration;
	struct IntegerPrintingConfiguration* integerPrintingConfirguration = &formatterFormatContext->integerPrintingConfiguration;

	assert(commonPrintingConfiguration->precision >= -1);
	assert(commonPrintingConfiguration->fieldWidth >= -1);

	char buffer[32];
	int length = 0;
	commonPrintingConfiguration->emmitNegativeSign = false;

	if (integerPrintingConfirguration->isHexadecimal) {
		bool foundFirstNonZero = false;
		uint32_t mask = 0xF0000000;
		int nibbleFirstBit = 32 - 4;

		do {
			uint32_t digit = integer & mask;
			digit = digit >> nibbleFirstBit;

			if (digit != 0) {
				foundFirstNonZero = true;
			}

			if (foundFirstNonZero || nibbleFirstBit == 0) {
				char c;
				if (digit < 10) {
					c = '0' + digit;
				} else {
					if (commonPrintingConfiguration->isUpperCase) {
						c = 'A';
					} else {
						c = 'a';
					}
					c += (digit - 10);
				}
				buffer[length++] = c;
			}

			mask = mask >> 4;
			nibbleFirstBit -= 4;

		} while (nibbleFirstBit >= 0);

	} else {
		if (integerPrintingConfirguration->isSigned && ((int) integer) < 0) {
			commonPrintingConfiguration->emmitNegativeSign = true;
			integer = -((int) integer);
		}

		uint32_t divisor = 1000000000;
		bool foundFirstNonZero = false;


		do {
			int digit = (int) integer / divisor;
			integer = integer % divisor;
			divisor /= 10;

			if (digit != 0 || divisor == 0) {
				foundFirstNonZero = true;
			}

			if (foundFirstNonZero)  {
				char c = '0' + digit;
				buffer[length++] = c;
			}

		} while(divisor != 0);
	}

	if (commonPrintingConfiguration->precision != -1 || commonPrintingConfiguration->fieldWidth != -1) {
		for (int i = mathUtilsMax(commonPrintingConfiguration->precision, commonPrintingConfiguration->fieldWidth); i > length; i--) {
			if (commonPrintingConfiguration->precision != -1 && i <= commonPrintingConfiguration->precision) {
				if (commonPrintingConfiguration->emmitNegativeSign) {
					emmitNegativeSign(formatterFormatContext);
				} else {
					emmitCharacter(formatterFormatContext, '0');
				}

			} else if (commonPrintingConfiguration->fieldWidth != -1 && i <= commonPrintingConfiguration->fieldWidth) {
				if (commonPrintingConfiguration->fillWithZeros) {
					if (commonPrintingConfiguration->emmitNegativeSign) {
						emmitNegativeSign(formatterFormatContext);
					} else {
						emmitCharacter(formatterFormatContext, '0');
					}

				} else {
					if (commonPrintingConfiguration->emmitNegativeSign &&
							/* Is it the last field width position that needs to be filled? */
							((commonPrintingConfiguration->precision == -1 && i - 1 <= length)
									|| (commonPrintingConfiguration->precision != -1 && i - 1 <= commonPrintingConfiguration->precision))) {
						emmitNegativeSign(formatterFormatContext);
					} else {
						emmitCharacter(formatterFormatContext, ' ');
					}
				}

			} else {
				assert(false);
			}
		}
	}

	if (commonPrintingConfiguration->emmitNegativeSign) {
		emmitNegativeSign(formatterFormatContext);
	}
	for (int i = 0; i < length; i++) {
		emmitCharacter(formatterFormatContext, buffer[i]);
	}
}

static void emmit64BitInteger(struct FormatterFormatContext* formatterFormatContext, uint64_t integer) {
	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext->commonPrintingConfiguration;
	struct IntegerPrintingConfiguration* integerPrintingConfirguration = &formatterFormatContext->integerPrintingConfiguration;

	char buffer[32];
	int length = 0;

	if (integerPrintingConfirguration->isHexadecimal) {
		bool foundFirstNonZero = false;
		uint64_t mask = 0xF000000000000000ULL;
		int nibbleFirstBit = 64 - 4;

		do {
			int digit = (int) ((integer & mask) >> nibbleFirstBit);

			if (digit != 0) {
				foundFirstNonZero = true;
			}

			if (foundFirstNonZero || nibbleFirstBit == 0) {
				char c;
				if (digit < 10) {
					c = '0' + digit;
				} else {
					if (commonPrintingConfiguration->isUpperCase) {
						c = 'A';
					} else {
						c = 'a';
					}
					c += (digit - 10);
				}
				buffer[length++] = c;
			}

			mask = mask >> 4;
			nibbleFirstBit -= 4;

		} while (nibbleFirstBit >= 0);

	} else {
		/* Not implemented yet. */
	}

	if (commonPrintingConfiguration->precision != -1 || commonPrintingConfiguration->fieldWidth != -1) {
		for (int i = mathUtilsMax(commonPrintingConfiguration->precision, commonPrintingConfiguration->fieldWidth); i > length; i--) {
			if (commonPrintingConfiguration->precision != -1 && i <= commonPrintingConfiguration->precision) {
				 emmitCharacter(formatterFormatContext,  '0');
			} else if (commonPrintingConfiguration->fieldWidth != -1 && i <= commonPrintingConfiguration->fieldWidth) {
				 emmitCharacter(formatterFormatContext,  commonPrintingConfiguration->fillWithZeros ? '0' : ' ');
			} else {
				assert(false);
			}
		}
	}
	for (int i = 0; i < length; i++) {
		emmitCharacter(formatterFormatContext, buffer[i]);
	}
}

static bool isContentLengthModifier(char c) {
	return c == 'l' || c == 'L';
}

static const char SPECIFIER_CHARACTERS_STRING[] = "dixXs%ucpPbfFgG";
static bool isSpecifierCharacter(char c) {
	return strchr(SPECIFIER_CHARACTERS_STRING, c) != NULL;
}

/*
 * Grammar:
 *
 * %[flags][width][.precision][length]specifier
 */

static char NULL_STRING[] = "(null)";
ssize_t formatterFormat(struct StreamWriter* streamWriter, const char* format, va_list ap, size_t* requiredLength) {
	int consumedCharacterCountBefore = streamWriterGetWrittenCharacterCount(streamWriter);
	bool foundPercentCharacter = false;
	int contentLengthInBytes = -1;

	struct FormatterFormatContext formatterFormatContext;
	memset(&formatterFormatContext, 0, sizeof(struct FormatterFormatContext));
	formatterFormatContext.streamWriter = streamWriter;

	struct IntegerPrintingConfiguration* integerPrintingConfiguration = &formatterFormatContext.integerPrintingConfiguration;

	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext.commonPrintingConfiguration;
	commonPrintingConfiguration->precision = -1;
	commonPrintingConfiguration->fieldWidth = -1;

	char character = '\0';
	while ((character = *format)) {
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

			} else if (character == '.') {
				format++;
				if (*format == '*') {
					format++;
					commonPrintingConfiguration->precision = va_arg(ap, int);
				} else {
					scannerParseInt32(format, 10, false, true, &format, &commonPrintingConfiguration->precision);
				}
				if (commonPrintingConfiguration->precision < -1) {
					commonPrintingConfiguration->precision = -1;
				}
				continue;

			} else if (character == '0') {
				commonPrintingConfiguration->fillWithZeros = true;

			} else if (character == '*') {
				commonPrintingConfiguration->fieldWidth = va_arg(ap, int);

			} else if (isdigit(character)) {
				scannerParseInt32(format, 10, false, true, &format, &commonPrintingConfiguration->fieldWidth);
				if (commonPrintingConfiguration->fieldWidth < -1) {
					commonPrintingConfiguration->fieldWidth = -1;
				}
				continue;

			} else if (isSpecifierCharacter(character)) {
				switch (*format) {
					case 'p':
					case 'P':
						{
							commonPrintingConfiguration->precision = sizeof(void*) * 2;

							uint32_t integer = (uint32_t) va_arg(ap, void*);
							integerPrintingConfiguration->isHexadecimal = true;
							commonPrintingConfiguration->isUpperCase = true;
							emmit32BitInteger(&formatterFormatContext, integer);
						}
					break;

					case 'x':
					case 'X':
					case 'u':
					case 'd':
					case 'i':
						{
							commonPrintingConfiguration->isUpperCase = character == 'X';
							if (contentLengthInBytes == 8) {
								uint64_t integer = va_arg(ap, uint64_t);
								integerPrintingConfiguration->isHexadecimal = true;
								integerPrintingConfiguration->isSigned = false;
								emmit64BitInteger(&formatterFormatContext, integer);

							} else {
								uint32_t integer = va_arg(ap, uint32_t);

								integerPrintingConfiguration->isHexadecimal = character == 'x' || character == 'X';
								integerPrintingConfiguration->isSigned = character == 'd' || character == 'i';

								emmit32BitInteger(&formatterFormatContext, integer);
							}
						}
					break;

					case 'G':
					case 'g':
						// TODO: Implement me!
					case 'F':
					case 'f':
						{
							commonPrintingConfiguration->isUpperCase = character == 'F';

							long double value;
							if (contentLengthInBytes == 8) {
								value = va_arg(ap, long double);
							} else {
								value = va_arg(ap, double);
							}

							emmitFloat(&formatterFormatContext, value);
						}
					break;


					case 'b':
						{
							bool value = (bool) va_arg(ap , int);
							const char* string = value ? "true" : "false";
							while(*string) {
								emmitCharacter(&formatterFormatContext, *string);
								string++;
							}
						}
						break;

					case 's':
						{
							char *string = va_arg(ap , char*);
							if (string == NULL) {
								string = NULL_STRING;
							}

							int length = -1;


							if (commonPrintingConfiguration->precision != -1) {
								length = 0;
								while (length < commonPrintingConfiguration->precision && string[length] != '\0') length++;
							} else {
								length = strlen(string);
							}
							if (commonPrintingConfiguration->fieldWidth != -1) {
								for (int i = length; i < commonPrintingConfiguration->fieldWidth; i++) {
									emmitCharacter(&formatterFormatContext, ' ');
								}
							}

							while((commonPrintingConfiguration->precision == -1 || commonPrintingConfiguration->precision > 0)
									&& (length == -1 || length > 0)) {
								emmitCharacter(&formatterFormatContext, *string);
								string++;
								if (commonPrintingConfiguration->fieldWidth != -1) {
									commonPrintingConfiguration->fieldWidth--;
								}
								length--;
							}
						}
					break;

					case 'c':
						emmitCharacter(&formatterFormatContext, va_arg(ap, int));
					break;

					case '%':
						emmitCharacter(&formatterFormatContext, '%');
					break;
				}

				foundPercentCharacter = false;
				contentLengthInBytes = -1;
				memset(integerPrintingConfiguration, 0, sizeof(struct IntegerPrintingConfiguration));

				memset(commonPrintingConfiguration, 0, sizeof(struct CommonPrintingConfiguration));
				commonPrintingConfiguration->precision = -1;
				commonPrintingConfiguration->fieldWidth = -1;
			}

		} else {
			if (character == '%') {
				foundPercentCharacter = true;

			} else {
				emmitCharacter(&formatterFormatContext, character);
			}
		}

		format++;
	}

	if (requiredLength != NULL) {
		*requiredLength = formatterFormatContext.requiredLength;
	}

	ssize_t result = streamWriterGetWrittenCharacterCount(streamWriter) - consumedCharacterCountBefore;
	if (result == 0) {
		if (formatterFormatContext.eof) {
			return EOF;
		} else {
			return 0;
		}

	} else {
		return result;
	}
}

static const char* FULL_MONTH_NAME[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

static const char* ABBREVIATED_MONTH_NAME[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

static const char* FULL_WEEKDAY_NAME[] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

static const char* ABBREVIATED_WEEKDAY_NAME[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

static void emmitArrayItemOrDefault(struct FormatterFormatContext* formatterFormatContext,
		const char** items, int itemCount, int itemIndex, const char* defaultValue) {
	const char* value;
	if (0 <= itemIndex && itemIndex < itemCount) {
		value = items[itemIndex];
	} else {
		value = defaultValue;
	}

	emmitString(formatterFormatContext, value);
}

void recursiveFormatDateTime(struct FormatterFormatContext* formatterFormatContext, struct StreamWriter* streamWriter, const char* format,
		const struct tm* tmInstance) {
	bool foundPercentCharacter = false;

	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext->commonPrintingConfiguration;

	char character = '\0';
		while ((character = *format)) {
			if (foundPercentCharacter) {
				if (character == 'E' || character == 'O') {

				} else {
					/* References:
					 * - https://cplusplus.com/reference/ctime/strftime/
					 * - https://man7.org/linux/man-pages/man3/strftime.3.html
					 */

					switch(character) {
						/* Abbreviated weekday name: */
						case 'a':
							emmitArrayItemOrDefault(formatterFormatContext, ABBREVIATED_WEEKDAY_NAME, 7, tmInstance->tm_wday, "?");
							break;

						/* Full weekday name: */
						case 'A':
							emmitArrayItemOrDefault(formatterFormatContext, FULL_WEEKDAY_NAME, 7, tmInstance->tm_wday, "?");
							break;

						/* Abbreviated month name: */
						case 'b':
						case 'h':
							emmitArrayItemOrDefault(formatterFormatContext, ABBREVIATED_MONTH_NAME, 12, tmInstance->tm_mon, "?");
							break;

						/* Full month name: */
						case 'B':
							emmitArrayItemOrDefault(formatterFormatContext, FULL_MONTH_NAME, 12, tmInstance->tm_mon, "?");
							break;

						/* Date and time representation: */
						case 'c':
							recursiveFormatDateTime(formatterFormatContext, streamWriter, "%a %b %e %H:%M:%S %Y", tmInstance);
							break;

						/* Day of the month, zero-padded (01-31): */
						case 'd':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 2;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_mday);
							break;

						/* Short MM/DD/YY date: */
						case 'D':
						case 'x':
							recursiveFormatDateTime(formatterFormatContext, streamWriter, "%m/%d/%y", tmInstance);
							break;

						/* Day of the month, space-padded ( 1-31): */
						case 'e':
							commonPrintingConfiguration->fieldWidth = 2;
							commonPrintingConfiguration->precision = -1;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_mday);
							break;

						/* Short YYYY-MM-DD date: */
						case 'F':
							recursiveFormatDateTime(formatterFormatContext, streamWriter, "%Y-%m-%d", tmInstance);
							break;

						/* Hour in 24h format (00-23): */
						case 'H':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 2;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_hour);
							break;

						/* Hour in 12h format (01-12): */
						case 'I':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 2;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_hour % 12);
							break;

						/* Day of the year: */
						case 'j':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 3;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_yday + 1);
							break;

						/* Hour in 24h format ( 0-23): */
						case 'k':
							commonPrintingConfiguration->fieldWidth = 2;
							commonPrintingConfiguration->precision = -1;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_hour);
							break;

						/* Hour in 12h format ( 1-12): */
						case 'l':
							commonPrintingConfiguration->fieldWidth = 2;
							commonPrintingConfiguration->precision = -1;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_hour % 12);
							break;

						/*	Month as a decimal number (01-12): */
						case 'm':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 2;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_mon + 1);
							break;

						/* Minute (00-59): */
						case 'M':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 2;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_min);
							break;

						case 'n':
							emmitCharacter(formatterFormatContext, '\n');
							break;

						/* AM or PM designation: */
						case 'p':
							emmitString(formatterFormatContext, tmInstance->tm_hour < 12 ? "AM" : "PM");
							break;

						/* am or pm designation: */
						case 'P':
							emmitString(formatterFormatContext, tmInstance->tm_hour < 12 ? "am" : "pm");
							break;

						/* 12-hour clock time: */
						case 'r':
							recursiveFormatDateTime(formatterFormatContext, streamWriter, "%I:%M:%S %p", tmInstance);
							break;

						/* 24-hour HH:MM time: */
						case 'R':
							recursiveFormatDateTime(formatterFormatContext, streamWriter, "%H:%M", tmInstance);
							break;

						/* Second (00-61): */
						case 'S':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = 2;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_sec);
							break;

						case 't':
							emmitCharacter(formatterFormatContext, '\t');
							break;

						/* ISO 8601 time format (HH:MM:SS): */
						case 'T':
						case 'X':
							recursiveFormatDateTime(formatterFormatContext, streamWriter, "%H:%M:%S", tmInstance);
							break;

						/* Year, last two digits: */
						case 'y':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = -1;
							emmit32BitInteger(formatterFormatContext, (tmInstance->tm_year + 1900) % 100);
							break;

						/* Year: */
						case 'Y':
							commonPrintingConfiguration->fieldWidth = -1;
							commonPrintingConfiguration->precision = -1;
							emmit32BitInteger(formatterFormatContext, tmInstance->tm_year + 1900);
							break;

						case 'z':
							emmitString(formatterFormatContext, "UTC");
							break;

						case 'Z':
							emmitString(formatterFormatContext, "+0000");
							break;

						case '%':
							emmitCharacter(formatterFormatContext, '%');
							break;
					}

					foundPercentCharacter = false;
				}

			} else {
				if (character == '%') {
					foundPercentCharacter = true;

				} else {
					emmitCharacter(formatterFormatContext, character);
				}
			}

			format++;
		}
}

ssize_t formatterFormatDateTime(struct StreamWriter* streamWriter, const char* format, const struct tm* tmInstance,
		bool emmitNullCharacter, size_t* requiredLength) {
	int consumedCharacterCountBefore = streamWriterGetWrittenCharacterCount(streamWriter);

	struct FormatterFormatContext formatterFormatContext;
	memset(&formatterFormatContext, 0, sizeof(struct FormatterFormatContext));
	formatterFormatContext.streamWriter = streamWriter;

	struct CommonPrintingConfiguration* commonPrintingConfiguration = &formatterFormatContext.commonPrintingConfiguration;
	commonPrintingConfiguration->precision = -1;
	commonPrintingConfiguration->fieldWidth = -1;

	recursiveFormatDateTime(&formatterFormatContext, streamWriter, format, tmInstance);

	if (emmitNullCharacter) {
		emmitCharacter(&formatterFormatContext, '\0');
	}

	if (requiredLength != NULL) {
		*requiredLength = formatterFormatContext.requiredLength;
	}

	ssize_t result = streamWriterGetWrittenCharacterCount(streamWriter) - consumedCharacterCountBefore;
	if (result == 0) {
		if (formatterFormatContext.eof) {
			return EOF;
		} else {
			return 0;
		}

	} else {
		return result;
	}
}
