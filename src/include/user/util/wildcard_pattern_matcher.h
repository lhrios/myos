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

#ifndef WILDCARD_PATTERN_MATCHER_H
	#define WILDCARD_PATTERN_MATCHER_H

	#include <fnmatch.h>
	#include <stdbool.h>

	#define FNM_ERROR 2

	#define WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY 16
	#define WILDCARD_PATTERN_MATCHER_STATIC_RANGES_CAPACITY 8

	struct WildcardPatternRange {
		char begin;
		char end;
	};

	enum WildcardPatternRuleType {
		WILDCARD_PATTERN_RULE_ASTERISK,
		WILDCARD_PATTERN_RULE_LIST,
		WILDCARD_PATTERN_RULE_LITERAL,
		WILDCARD_PATTERN_RULE_QUESTION_MARK
	};

	struct WildcardPatternList {
		enum WildcardPatternRuleType patternRuleType;

		char* characters;
		int characterCount;
		int characterCapacity;

		struct WildcardPatternRange* ranges;
		int rangeCount;
		int rangeCapacity;

		int namedClasses;

		char staticCharacters[WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY];
		struct WildcardPatternRange staticRanges[WILDCARD_PATTERN_MATCHER_STATIC_RANGES_CAPACITY];

		bool complement;
	};

	struct WildcardPatternLiteral {
		enum WildcardPatternRuleType patternRuleType;
		char* characters;
		int characterCount;
		int characterCapacity;

		char staticCharacters[WILDCARD_PATTERN_MATCHER_STATIC_CHARACTERS_CAPACITY];
	};

	union WildcardPatternRule {
		enum WildcardPatternRuleType patternRuleType;
		struct WildcardPatternList list;
		struct WildcardPatternLiteral literal;
	};

	#define WILDCARD_PATTERN_MATCHER_STATIC_PATTERN_RULES_CAPACITY 16

	struct WildcardPattern {
		union WildcardPatternRule* patternRules;
		int patternRuleCount;
		int patternRuleCapacity;

		union WildcardPatternRule staticPatternRules[WILDCARD_PATTERN_MATCHER_STATIC_PATTERN_RULES_CAPACITY];

		int flags;
		bool caseSensitive;

		bool usesDynamicAllocatedMemory;
	};

	void wildcardPatternMatcherReleasePattern(struct WildcardPattern* pattern);
	bool wildcardPatternMatcherParsePattern(const char* rawPattern, struct WildcardPattern* pattern, int flags);

	bool wildcardPatternMatcherMatch(const char* string, struct WildcardPattern* pattern);
	bool wildcardPatternMatcherCompileAndMatch(const char* rawPattern, const char* string, int flags);

#endif
