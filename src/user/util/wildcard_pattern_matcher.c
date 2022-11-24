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
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "user/util/wildcard_pattern_matcher.h"

#include "util/math_utils.h"

//#define DEBUG_PATTERN_PARSING

const char* PATTERN_RULE_TYPE_NAMES[] = {
	"ASTERISK",
	"LIST",
	"LITERAL",
	"QUESTION_MARK"
};

enum NamedClassType {
	ALPHANUMERIC,
	ALPHABETIC,
	BLANK,
	CONTROL,
	DIGIT,
	GRAPHICAL,
	LOWER_CASE_LETTER,
	PRINTABLE,
	PUNCTUATION,
	SPACE,
	UPPER_CASE_LETTER,
	HEXADECIMAL,
	UNKNOWN
};

const char* NAMED_CLASS_TYPE_NAMES[] = {
	"ALPHANUMERIC",
	"ALPHABETIC",
	"BLANK",
	"CONTROL",
	"DIGIT",
	"GRAPHICAL",
	"LOWER_CASE_LETTER",
	"PRINTABLE",
	"PUNCTUATION",
	"SPACE",
	"UPPER_CASE_LETTER",
	"HEXADECIMAL",
	"UNKNOWN"
};

struct WildcardPatternPreparationContext {
	int currentIndex;

	const char* rawPattern;
	int rawPatternLength;

	const char* namedClassNameBegin;
	int namedClassNameLength;

	int squareBracketOpenCount;
	bool escaping;

	char current;

	struct WildcardPattern* pattern;
};

static char lookahead(struct WildcardPatternPreparationContext* context, int distance) {
	int index = context->currentIndex + distance;
	if (index < context->rawPatternLength) {
		return context->rawPattern[index];
	} else {
		return '\0';
	}
}

static bool pushElement(struct WildcardPatternPreparationContext* context, void** elements, void* staticElements,
		int staticElementsCapacity, int* elementCount, int* elementCapacity, size_t elementSize, void* element) {
	bool willExceedCapacity;
	int offset;
	if (*elementCount + 1 > *elementCapacity) {
		willExceedCapacity = true;
		if (*elements == staticElements) {
			offset = 0;
		} else {
			assert(*elementCount >= staticElementsCapacity);
			offset = *elementCount - staticElementsCapacity;
		}

	} else {
		willExceedCapacity = false;
		if (*elements == staticElements) {
			offset = *elementCount;
		} else {
			assert(*elementCount >= staticElementsCapacity);
			offset = *elementCount - staticElementsCapacity;
		}
	}
	assert(offset >= 0);

	if (willExceedCapacity) {
		(*elementCapacity) *= 2;
		if (staticElements == *elements) {
			context->pattern->usesDynamicAllocatedMemory = true;
			*elements = malloc(elementSize * *elementCapacity);

		} else {
			*elements = realloc(*elements, elementSize * *elementCapacity);
		}
	}

	if (elements != NULL) {
		if (element != NULL) {
			memcpy(*elements + offset * elementSize, element, elementSize);
		}
		(*elementCount)++;
		return true;
	} else {
		return false;
	}
}

static void* getElement(void* elements, void* staticElements, int staticElementsCapacity, size_t elementSize, int index) {
	if (index < staticElementsCapacity) {
		return staticElements + index * elementSize;
	} else {
		return elements + (index - staticElementsCapacity) * elementSize;
	}
}

static bool pushCharacterToLiteral(struct WildcardPatternPreparationContext* context, struct WildcardPatternLiteral* literal) {
	return pushElement(context, (void**) &literal->characters, literal->staticCharacters, WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY,
		&literal->characterCount, &literal->characterCapacity, sizeof(char), &context->current);
}

static bool pushCharacterToList(struct WildcardPatternPreparationContext* context, struct WildcardPatternList* list) {
	struct WildcardPattern* pattern = context->pattern;
	bool caseSensitive = pattern->caseSensitive;
	char character = caseSensitive ? context->current : tolower(context->current);
	return pushElement(context, (void**) &list->characters, list->staticCharacters, WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY,
		&list->characterCount, &list->characterCapacity, sizeof(char), &character);
}

static bool pushIntervalToList(struct WildcardPatternPreparationContext* context, struct WildcardPatternList* list, char being, char end) {
	bool caseSensitive = context->pattern->caseSensitive;
	struct WildcardPatternRange range;
	range.begin = caseSensitive ? being : tolower(being);
	range.end = caseSensitive ? end : tolower(end);

	return pushElement(context, (void**) &list->ranges, list->staticRanges, WILDCARD_PATTERN_MATCHER_STATIC_RANGES_CAPACITY,
		&list->rangeCount, &list->rangeCapacity, sizeof(struct WildcardPatternRange), (void*) &range);
}

static union WildcardPatternRule* getPatternRule(struct WildcardPattern* pattern, int index) {
	return getElement(pattern->patternRules, pattern->staticPatternRules, WILDCARD_PATTERN_MATCHER_STATIC_PATTERN_RULES_CAPACITY, sizeof(union WildcardPatternRule), index);
}

static union WildcardPatternRule* peekPatternRule(struct WildcardPatternPreparationContext* context) {
	if (context->pattern->patternRuleCount > 0) {
		return getPatternRule(context->pattern, context->pattern->patternRuleCount - 1);
	} else {
		return NULL;
	}
}

static union WildcardPatternRule* pushPatternRule(struct WildcardPatternPreparationContext* context) {
	struct WildcardPattern* pattern = context->pattern;

	bool result = pushElement(context, (void**) &pattern->patternRules, pattern->staticPatternRules, WILDCARD_PATTERN_MATCHER_STATIC_PATTERN_RULES_CAPACITY,
		&pattern->patternRuleCount, &pattern->patternRuleCapacity, sizeof(union WildcardPatternRule), NULL);

	if (result) {
		#ifdef DEBUG_PATTERN_PARSING
			fprintf(stderr, "successfully pushed a pattern rule\n");
		#endif

		return peekPatternRule(context);
	} else {
		return NULL;
	}
}

static bool matchAsteriskOrQuestionMark(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchAsteriskOrQuestionMark %c\n", context->current);
	#endif

	union WildcardPatternRule* patternRule = NULL;

	if (context->current == '*') {
		patternRule = peekPatternRule(context);
		if (patternRule == NULL || patternRule->patternRuleType != WILDCARD_PATTERN_RULE_ASTERISK) {
			patternRule = pushPatternRule(context);
			if (patternRule != NULL) {
				patternRule->patternRuleType = WILDCARD_PATTERN_RULE_ASTERISK;
			}
		}

	} else {
		assert(context->current == '?');
		patternRule = pushPatternRule(context);
		if (patternRule != NULL) {
			patternRule->patternRuleType = WILDCARD_PATTERN_RULE_QUESTION_MARK;
		}
	}

	context->currentIndex++;

	return patternRule != NULL;
}

static enum NamedClassType namedClassTypeFromName(const char* namedClassNameBegin, int namedClassNameLength) {
	if (strncmp("alnum", namedClassNameBegin, namedClassNameLength) == 0) {
		return ALPHANUMERIC;

	} else if (strncmp("alpha", namedClassNameBegin, namedClassNameLength) == 0) {
		return ALPHABETIC;

	} else if (strncmp("blank", namedClassNameBegin, namedClassNameLength) == 0) {
		return BLANK;

	} else if (strncmp("cntrl", namedClassNameBegin, namedClassNameLength) == 0) {
		return CONTROL;

	} else if (strncmp("digit", namedClassNameBegin, namedClassNameLength) == 0) {
		return DIGIT;

	} else if (strncmp("graph", namedClassNameBegin, namedClassNameLength) == 0) {
		return GRAPHICAL;

	} else if (strncmp("lower", namedClassNameBegin, namedClassNameLength) == 0) {
		return LOWER_CASE_LETTER;

	} else if (strncmp("print", namedClassNameBegin, namedClassNameLength) == 0) {
		return PRINTABLE;

	} else if (strncmp("punct", namedClassNameBegin, namedClassNameLength) == 0) {
		return PUNCTUATION;

	} else if (strncmp("space", namedClassNameBegin, namedClassNameLength) == 0) {
		return SPACE;

	} else if (strncmp("upper", namedClassNameBegin, namedClassNameLength) == 0) {
		return UPPER_CASE_LETTER;

	} else if (strncmp("xdigit", namedClassNameBegin, namedClassNameLength) == 0) {
		return HEXADECIMAL;

	} else {
		return UNKNOWN;
	}
}

static void pushNamedClassToList(struct WildcardPatternList* list, const char* namedClassNameBegin, int namedClassNameLength) {
	enum NamedClassType namedClassType = namedClassTypeFromName(namedClassNameBegin, namedClassNameLength);
	list->namedClasses |= (1 << namedClassType);
}

static bool matchLiteral(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchLiteral %c\n", context->current);
	#endif

	bool result = true;

	union WildcardPatternRule* patternRule = peekPatternRule(context);
	struct WildcardPatternLiteral* literal = NULL;
	if (patternRule == NULL || patternRule->patternRuleType != WILDCARD_PATTERN_RULE_LITERAL) {
		patternRule = pushPatternRule(context);
		if (patternRule != NULL) {
			literal = &patternRule->literal;
			literal->patternRuleType = WILDCARD_PATTERN_RULE_LITERAL;
			literal->characters = literal->staticCharacters;
			literal->characterCapacity = WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY;
			literal->characterCount = 0;
		} else {
			result = false;
		}
	} else {
		literal = &patternRule->literal;
	}

	if (result) {
		result = pushCharacterToLiteral(context, literal);
	}

	context->escaping = false;
	context->currentIndex++;

	return result;
}

static bool matchOpenList(struct WildcardPatternPreparationContext* context) {
	union WildcardPatternRule* patternRule = pushPatternRule(context);
	struct WildcardPatternList* list = NULL;
	if (patternRule != NULL) {
		list = &patternRule->list;

		list->patternRuleType = WILDCARD_PATTERN_RULE_LIST;
		list->complement = false;

		list->characterCount = 0;
		list->characters = list->staticCharacters;
		list->characterCapacity = WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY;

		list->rangeCount = 0;
		list->ranges = list->staticRanges;
		list->rangeCapacity = WILDCARD_PATTERN_MATCHER_STATIC_RANGES_CAPACITY;

		list->namedClasses = 0;

		if (lookahead(context, 1) == '!') {
			#ifdef DEBUG_PATTERN_PARSING
				fprintf(stderr, "matchOpenList with complement\n");
			#endif
			list->complement = true;
			context->currentIndex += 2;
		} else {
			#ifdef DEBUG_PATTERN_PARSING
				fprintf(stderr, "matchOpenList\n");
			#endif
			context->currentIndex++;
		}
	}

	context->squareBracketOpenCount++;
	return list != NULL;
}

static bool isListEmpty(struct WildcardPatternList* list) {
	return list->characterCount == 0 && list->rangeCount == 0 && list->namedClasses == 0;
}

struct WildcardPatternRange* getRange(struct WildcardPatternList* list, int index) {
	return getElement(list->ranges, list->staticRanges, WILDCARD_PATTERN_MATCHER_STATIC_RANGES_CAPACITY, sizeof(struct WildcardPatternRange), index);
}

#ifdef DEBUG_PATTERN_PARSING

	static void printList(struct WildcardPatternList* list) {
		if (list->complement){
			fprintf(stderr, "\tcomplement\n");
		}
		if (list->characterCount <= WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY) {
			fprintf(stderr, "\tcharacters: %.*s\n", (int) list->characterCount, list->characters);
		} else {
			fprintf(stderr, "\tcharacters: %.*s", WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY, list->staticCharacters);
			fprintf(stderr, "%.*s\n", (int) list->characterCount - WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY, list->characters);
		}

		fprintf(stderr, "\tranges:\n");
		for (int i = 0; i < list->rangeCount; i++) {
			struct WildcardPatternRange* range = getRange(list, i);
			fprintf(stderr, "\t\trange %d: from %c to %c (inclusive)\n", i, range->begin, range->end);
		}
		if (list->namedClasses != 0) {
			fprintf(stderr, "\tnamed classes:\n");
			for (int i = 0; i != UNKNOWN; i++) {
				if (list->namedClasses & (1 << i)) {
					fprintf(stderr, "\t\t%s\n", NAMED_CLASS_TYPE_NAMES[i]);
				}
			}
		}
	}

	static void printLiteral(struct WildcardPatternLiteral* literal) {
		if (literal->characterCount <= WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY) {
			fprintf(stderr, "\tcharacters: %.*s\n", literal->characterCount, literal->characters);
		} else {
			printf("%d\n", literal->characterCount);

			assert(literal->staticCharacters != literal->characters);
			fprintf(stderr, "\tcharacters: %.*s", WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY, literal->staticCharacters);
			fprintf(stderr, "%.*s\n", literal->characterCount - WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY, literal->characters);
		}
	}

	static void printPattern(struct WildcardPattern* pattern) {
		for (int i = 0; i < pattern->patternRuleCount; i++) {
			union WildcardPatternRule* patternRule = getPatternRule(pattern, i);
			fprintf(stderr, "rule %s (%d)", PATTERN_RULE_TYPE_NAMES[patternRule->patternRuleType], i);
			if (patternRule->patternRuleType == WILDCARD_PATTERN_RULE_LIST) {
				fprintf(stderr, ":\n");
				printList(&patternRule->list);
			} else if (patternRule->patternRuleType == WILDCARD_PATTERN_RULE_LITERAL) {
				fprintf(stderr, ":\n");
				printLiteral(&patternRule->literal);
			} else {
				fprintf(stderr, "\n");
			}
		}
	}

#endif

static void matchCloseList(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchCloseList\n");
	#endif

	context->squareBracketOpenCount--;
	context->currentIndex++;
}

static bool matchLiteralInsideList(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchLiteralInsideList %c\n", context->current);
	#endif

	union WildcardPatternRule* patternRule = peekPatternRule(context);
	assert(patternRule != NULL && patternRule->patternRuleType == WILDCARD_PATTERN_RULE_LIST);
	struct WildcardPatternList* list = &patternRule->list;

	context->currentIndex++;
	return pushCharacterToList(context, list);
}

static bool matchInterval(struct WildcardPatternPreparationContext* context, char being, char end) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchInterval %c-%c\n", being, end);
	#endif

	union WildcardPatternRule* patternRule = peekPatternRule(context);
	assert(patternRule != NULL && patternRule->patternRuleType == WILDCARD_PATTERN_RULE_LIST);
	struct WildcardPatternList* list = &patternRule->list;

	context->currentIndex += 3;
	return pushIntervalToList(context, list, being, end);
}

static bool matchIntervalOrLiteralInsideList(struct WildcardPatternPreparationContext* context) {
	char lookahead1 = lookahead(context, 1);
	char lookahead2 = lookahead(context, 2);
	context->escaping = false;
	if (lookahead1 == '-' && lookahead2 != ']' && lookahead2 != '[' && lookahead2 != '\0') {
		return matchInterval(context, context->current, lookahead2);
	} else {
		return matchLiteralInsideList(context);
	}
}

static void matchOpenNamedClass(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchOpenNamedClass\n");
	#endif

	context->squareBracketOpenCount++;
	context->currentIndex += 2;
	context->namedClassNameBegin = context->rawPattern + context->currentIndex;
	context->namedClassNameLength = 0;
}

static void matchNamedClassCharacter(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchNamedClassCharacter %c\n", context->current);
	#endif

	context->currentIndex++;
	context->namedClassNameLength++;
}

static void matchCloseNamedClass(struct WildcardPatternPreparationContext* context) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "matchCloseNamedClass\n");
	#endif

	union WildcardPatternRule* patternRule = peekPatternRule(context);
	assert(patternRule != NULL && patternRule->patternRuleType == WILDCARD_PATTERN_RULE_LIST);
	struct WildcardPatternList* list = &patternRule->list;

	context->squareBracketOpenCount--;
	context->currentIndex += 2;

	pushNamedClassToList(list, context->namedClassNameBegin, context->namedClassNameLength);
}

bool wildcardPatternMatcherParsePattern(const char* rawPattern, struct WildcardPattern* pattern, int flags) {
	bool result = true;

	struct WildcardPatternPreparationContext context;
	memset(&context, 0, sizeof(struct WildcardPatternPreparationContext));
	context.rawPattern = rawPattern;
	context.rawPatternLength = strlen(rawPattern);
	context.pattern = pattern;

	pattern->flags = flags;
	pattern->caseSensitive  = (flags & FNM_CASEFOLD) == 0;
	pattern->usesDynamicAllocatedMemory = false;
	pattern->patternRuleCount = 0;
	pattern->patternRules = pattern->staticPatternRules;
	pattern->patternRuleCapacity = WILDCARD_PATTERN_MATCHER_STATIC_PATTERN_RULES_CAPACITY;

	while (result && context.currentIndex < context.rawPatternLength) {
		switch (context.current = context.rawPattern[context.currentIndex]) {
			case '?':
			case '*':
				if (context.squareBracketOpenCount == 0) {
					if (context.escaping) {
						result = matchLiteral(&context);
					} else {
						result = matchAsteriskOrQuestionMark(&context);
					}

				} else if (context.squareBracketOpenCount == 1) {
					result = matchIntervalOrLiteralInsideList(&context);

				} else {
					matchNamedClassCharacter(&context);
				}
				break;

			case '!':
				if (context.squareBracketOpenCount == 0) {
					result = matchLiteral(&context);

				} else if (context.squareBracketOpenCount == 1) {
					result = matchIntervalOrLiteralInsideList(&context);

				} else {
					matchNamedClassCharacter(&context);
				}
				break;

			case '\\':
				if (context.squareBracketOpenCount == 0) {
					if ((flags & FNM_NOESCAPE) != 0 || context.escaping) {
						result = matchLiteral(&context);
					} else {
						context.escaping = true;
						context.currentIndex++;
					}

				} else if (context.squareBracketOpenCount == 1) {
					if ((flags & FNM_NOESCAPE) != 0 || context.escaping) {
						result = matchIntervalOrLiteralInsideList(&context);
					} else {
						context.escaping = true;
						context.currentIndex++;
					}

				} else {
					matchNamedClassCharacter(&context);
				}
				break;

			case '[':
				if (context.squareBracketOpenCount == 0) {
					if (context.escaping) {
						result = matchLiteral(&context);
					} else {
						result = matchOpenList(&context);
					}

				} else if (context.squareBracketOpenCount == 1) {
					if (context.escaping || lookahead(&context, 1) != ':') {
						result = matchIntervalOrLiteralInsideList(&context);
					} else {
						matchOpenNamedClass(&context);
					}

				} else {
					matchNamedClassCharacter(&context);
				}
			break;

			case ']':
				if (context.squareBracketOpenCount == 0) {
					result = matchLiteral(&context);

				} else if (context.squareBracketOpenCount == 1) {
					if (context.escaping) {
						result = matchIntervalOrLiteralInsideList(&context);
					} else {
						union WildcardPatternRule* patternRule = peekPatternRule(&context);
						assert(patternRule != NULL && patternRule->patternRuleType == WILDCARD_PATTERN_RULE_LIST);
						struct WildcardPatternList* list = &patternRule->list;

						if (isListEmpty(list)) {
							result = matchIntervalOrLiteralInsideList(&context);
						} else {
							matchCloseList(&context);
						}
					}

				} else {
					matchNamedClassCharacter(&context);
				}
				break;

			case ':':
				if (context.squareBracketOpenCount == 0) {
					result = matchLiteral(&context);

				} else if (context.squareBracketOpenCount == 1) {
					result = matchIntervalOrLiteralInsideList(&context);

				} else {
					if (lookahead(&context, 1) == ']') {
						matchCloseNamedClass(&context);
					} else {
						matchNamedClassCharacter(&context);
					}
				}
			break;

			default:
				if (context.squareBracketOpenCount == 0) {
					result = matchLiteral(&context);

				} else if (context.squareBracketOpenCount == 1) {
					result = matchIntervalOrLiteralInsideList(&context);

				} else {
					matchNamedClassCharacter(&context);
				}
				break;
		}
	}

	return result;
}

static bool doesNamedClassContainCharacter(enum NamedClassType namedClassType, char character) {
	switch (namedClassType) {
		case ALPHANUMERIC:
			return isalnum(character);

		case ALPHABETIC:
			return isalpha(character);

		case BLANK:
			return isblank(character);

		case CONTROL:
			return iscntrl(character);

		case DIGIT:
			return isdigit(character);

		case GRAPHICAL:
			return isgraph(character);

		case LOWER_CASE_LETTER:
			return islower(character);

		case PRINTABLE:
			return isprint(character);

		case PUNCTUATION:
			return ispunct(character);

		case SPACE:
			return isspace(character);

		case UPPER_CASE_LETTER:
			return isupper(character);

		case HEXADECIMAL:
			return isxdigit(character);

		default:
			return false;
	}
}

static bool doesListContainCharacter(struct WildcardPatternList* list, char rawCharacter, bool caseSensitive) {
	bool result = false;
	char cookedCharacter = caseSensitive ? rawCharacter : tolower(rawCharacter);

	if (list->characterCount > 0 && !result) {
		result = memchr(list->staticCharacters, cookedCharacter, mathUtilsMin(list->characterCount, WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY)) != NULL;
		result = result || (list->staticCharacters != list->characters &&
			memchr(list->characters, cookedCharacter, list->characterCount - WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY) != NULL);
	}

	for (int i = 0; i < list->rangeCount && !result; i++) {
		struct WildcardPatternRange* range = getRange(list, i);
		result = range->begin <= cookedCharacter && cookedCharacter <= range->end;
	}

	int namedClasses = list->namedClasses;
	if (namedClasses != 0) {
		for (int i = 0; i != UNKNOWN && !result; i++) {
			if ((namedClasses & (1 << i)) != 0) {
				result = doesNamedClassContainCharacter(i, rawCharacter);
			}
		}
	}

	return list->complement ? !result : result;
}

static bool doesLiteralMatchString(struct WildcardPatternLiteral* literal, const char* string, size_t stringLength, bool caseSensitive) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "doesLiteralMatchString(%s, %d)\n", string, stringLength);
	#endif

	size_t characterCount = literal->characterCount;
	const char* staticCharacters = literal->staticCharacters;
	const char* characters = literal->characters;
	int (*compare)(const char *, const char *, size_t) = caseSensitive ? strncmp : strncasecmp;
	if (stringLength >= characterCount) {
		if (characterCount <= WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY) {
			return compare(staticCharacters, string, characterCount) == 0;
		} else {
			return compare(staticCharacters, string, WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY) == 0
				&& compare(characters, string + WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY, characterCount - WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY) == 0;
		}

	} else {
		return false;
	}
}

static bool canNonLiteralPatterRuleMatchPeriod(struct WildcardPattern* pattern, const char* string, int nextCharacterIndex) {
	int flags = pattern->flags;
	if ((flags & FNM_PERIOD) != 0) {
		bool isLeadingPeriod = nextCharacterIndex == 0
			|| (((flags & FNM_PATHNAME) != 0) && string[nextCharacterIndex - 1] == '/');
		return !isLeadingPeriod;
	} else {
		return true;
	}
}

static bool recursiveMatch(const char* string, int nextCharacterIndex, int unmatchedStringLength,
		struct WildcardPattern* pattern, int nextPatternRuleIndex) {
	#ifdef DEBUG_PATTERN_PARSING
		fprintf(stderr, "recursiveMatch(%s, %d)\n", string, nextPatternRuleIndex);
	#endif

	int flags = pattern->flags;
	bool caseSensitive = pattern->caseSensitive;
	int patternRuleCount = pattern->patternRuleCount;
	if (unmatchedStringLength >= 0 && nextPatternRuleIndex < patternRuleCount) {
		char c = unmatchedStringLength == 0 ? '\0' : string[nextCharacterIndex];

		union WildcardPatternRule* patternRule = getPatternRule(pattern, nextPatternRuleIndex);
		switch (patternRule->patternRuleType) {
			case WILDCARD_PATTERN_RULE_ASTERISK: {
				bool caseConsumeOneCharacter = unmatchedStringLength > 0 && (c != '/' || (flags & FNM_PATHNAME) == 0)
					&& (c != '.' || canNonLiteralPatterRuleMatchPeriod(pattern, string, nextCharacterIndex))
					&& recursiveMatch(string, nextCharacterIndex + 1, unmatchedStringLength - 1, pattern, nextPatternRuleIndex);
				return caseConsumeOneCharacter || recursiveMatch(string, nextCharacterIndex, unmatchedStringLength, pattern, nextPatternRuleIndex + 1);
			} break;

			case WILDCARD_PATTERN_RULE_QUESTION_MARK:
				return unmatchedStringLength > 0 && (c != '/' || (flags & FNM_PATHNAME) == 0)
						&& (c != '.' || canNonLiteralPatterRuleMatchPeriod(pattern, string, nextCharacterIndex))
						&& recursiveMatch(string, nextCharacterIndex + 1, unmatchedStringLength - 1, pattern, nextPatternRuleIndex + 1);

			case WILDCARD_PATTERN_RULE_LIST:
				if (unmatchedStringLength > 0 && (c != '/' || (flags & FNM_PATHNAME) == 0)
						&& (c != '.' || canNonLiteralPatterRuleMatchPeriod(pattern, string, nextCharacterIndex))
						&& doesListContainCharacter(&patternRule->list, c, caseSensitive)) {
					return recursiveMatch(string, nextCharacterIndex + 1, unmatchedStringLength - 1, pattern, nextPatternRuleIndex + 1);
				} else {
					return false;
				}
				break;

			case WILDCARD_PATTERN_RULE_LITERAL: {
				struct WildcardPatternLiteral* literal = &patternRule->literal;
				int characterCount = literal->characterCount;
				if (doesLiteralMatchString(literal, string + nextCharacterIndex, unmatchedStringLength, caseSensitive)) {
					return recursiveMatch(string, nextCharacterIndex + characterCount, unmatchedStringLength - characterCount,
						pattern, nextPatternRuleIndex + 1);
				} else {
					return false;
				}
			} break;

			default:
				assert(false);
				return false;
		}

	} else {
		return unmatchedStringLength == 0 && nextPatternRuleIndex == patternRuleCount;
	}
}

void wildcardPatternMatcherReleasePattern(struct WildcardPattern* pattern) {
	if (pattern->usesDynamicAllocatedMemory) {
		for (int i = 0; i < pattern->patternRuleCount; i++) {
			union WildcardPatternRule* patternRule = getPatternRule(pattern, i);

			switch (patternRule->patternRuleType) {
				case WILDCARD_PATTERN_RULE_LIST: {
					struct WildcardPatternList* list = &patternRule->list;
					if (list->characters != NULL && list->characters != list->staticCharacters) {
						free(list->characters);
					}
					if (list->ranges != NULL && list->ranges != list->staticRanges) {
						free(list->ranges);
					}

				} break;

				case WILDCARD_PATTERN_RULE_LITERAL: {
					struct WildcardPatternLiteral* literal = &patternRule->literal;
					if (literal->characters != NULL && literal->characters != literal->staticCharacters) {
						free(literal->characters);
					}

				} break;

				default:
					break;
			}
		}

		if (pattern->patternRules != NULL && pattern->patternRules != pattern->staticPatternRules) {
			free(pattern->patternRules);
		}
	}
}

bool wildcardPatternMatcherMatch(const char* string, struct WildcardPattern* pattern) {
	#ifdef DEBUG_PATTERN_PARSING
		printPattern(pattern);
	#endif

	bool result;
	if (pattern->flags & FNM_LEADING_DIR) {
		const char* firstSlash = strchr(string, '/');
		if (firstSlash == NULL) {
			result = recursiveMatch(string, 0, strlen(string), pattern, 0);
		} else {
			result = recursiveMatch(string, 0, firstSlash - string, pattern, 0);
		}
	} else {
		result = recursiveMatch(string, 0, strlen(string), pattern, 0);
	}

	return result;
}

bool wildcardPatternMatcherCompileAndMatch(const char* rawPattern, const char* string, int flags) {
	int result;

	struct WildcardPattern pattern;
	if (wildcardPatternMatcherParsePattern(rawPattern, &pattern, flags)) {
		if (string != NULL) {
			result = wildcardPatternMatcherMatch(string, &pattern) ? 0 : FNM_NOMATCH;
		} else {
			result = FNM_NOMATCH;
		}
		wildcardPatternMatcherReleasePattern(&pattern);

	} else {
		result = FNM_ERROR;
	}

	return result;
}
