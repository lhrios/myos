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
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "test/integration_test.h"

static char* trimAndCopy(const char* string) {
	int begin = 0;
	int end = strlen(string) - 1;

	while (begin <= end && (isspace(string[begin]) || isspace(string[end]))) {
		if (isspace(string[begin])) {
			begin++;
		}
		if (isspace(string[end])) {
			end--;
		}
	}

	if (begin > end) {
		return NULL;
	} else {
		size_t length = end - begin + 1;
		char* copy = malloc(sizeof(char) * (length + 1));
		assert(copy != NULL);
		strncpy(copy, string + begin, length);
		copy[length] = '\0';
		return copy;
	}
}

static bool isComment(const char* string) {
	return string != NULL && strlen(string) >= 2 && string[0] == '#' && string[1] == '#';
}

static bool isGroupBegin(const char* string) {
	return string != NULL && strlen(string) >= 1 && string[0] == '#';
}

struct Group {
	const char* name;
	const char** items;
	int itemCount;
	int itemCapacity;
};

static struct Group* createGroup(char* name) {
	struct Group* group = malloc(sizeof(struct Group));
	assert(group != NULL);
	memset(group, 0, sizeof(struct Group));
	group->name = name;
	return group;
}

static struct Group* addItem(struct Group* group, const char* item) {
	if (group->itemCapacity < group->itemCount + 1) {
		group->itemCapacity = group->itemCapacity == 0 ? 10 : group->itemCapacity * 2;
		group->items = realloc(group->items, sizeof(char*) * group->itemCapacity);
		assert(group->items != NULL);
	}
	group->items[group->itemCount++] = item;

	return group;
}


struct GroupList {
	struct Group** groups;
	int groupCount;
	int groupCapacity;
};

static struct GroupList* createGroupList() {
	struct GroupList* groupList = malloc(sizeof(struct GroupList));
	assert(groupList != NULL);
	memset(groupList, 0, sizeof(struct GroupList));
	return groupList;
}

static struct GroupList* addGroup(struct GroupList* groupList, struct Group* group) {
	if (groupList->groupCapacity < groupList->groupCount + 1) {
		groupList->groupCapacity = groupList->groupCapacity == 0 ? 10 : groupList->groupCapacity * 2;
		groupList->groups = realloc(groupList->groups, sizeof(struct Group) * groupList->groupCapacity);
		assert(groupList->groups != NULL);
	}
	groupList->groups[groupList->groupCount++] = group;

	return groupList;
}

static struct GroupList* readGroups(const char* filePath) {
	FILE* file = fopen(filePath, "r");
	assert(file != NULL);

	const size_t bufferSize = 1024;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);

	struct GroupList* groupList = createGroupList();
	struct Group* group = NULL;
	char* line;
	while ((line = fgets(buffer, bufferSize, file)) != NULL) {
		char* trimmedCopy = trimAndCopy(line);
		if (trimmedCopy != NULL) {
			if (!isComment(trimmedCopy)) {
				if (isGroupBegin(trimmedCopy)) {
					char* groupName = trimAndCopy(trimmedCopy + 1);
					assert(groupName != NULL && strlen(groupName) > 0);
					group = createGroup(groupName);
					groupList = addGroup(groupList, group);

				} else {
					addItem(group, trimmedCopy);
				}
			}
		}
	}
	free(buffer);
	fclose(file);

	return groupList;
}

static void createDirectories(struct GroupList* groupList) {
	const size_t bufferSize = 1024;
	char* buffer = malloc(sizeof(char) * bufferSize);
	assert(buffer != NULL);

	for (int groupIndex = 0; groupIndex < groupList->groupCount; groupIndex++) {
		struct Group* group = groupList->groups[groupIndex];

		sprintf(buffer, "%s", group->name);
		if (mkdir(buffer, S_IRWXU) != 0) {
			assert(errno == EEXIST);
		}

		for (int itemIndex = 0; itemIndex < group->itemCount; itemIndex++) {
			const char* item = group->items[itemIndex];

			sprintf(buffer, "%s/%c", group->name, item[0]);
			if (mkdir(buffer, S_IRWXU) != 0) {
				assert(errno == EEXIST);
			}

			assert(strlen(group->name) >= 3);
			sprintf(buffer, "%s/%c/%s.%c%c%c", group->name, item[0], item,
				tolower(group->name[0]), tolower(group->name[1]), tolower(group->name[2]));
			int fileDescriptorIndex = open(buffer, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
			assert(fileDescriptorIndex >= 0);
			close(fileDescriptorIndex);
		}
	}

	free(buffer);
}

static bool areEqual(int elementCount1, const char* const* elements1, int elementCount2, char* const* elements2) {
	if (elementCount1 == elementCount2) {
		for (int i = 0; i < elementCount1; i++) {
			const char* element1 = elements1[i];
			const char* element2 = elements2[i];
			if (strcmp(element1, element2) != 0) {
				return false;
			}
		}
		return true;

	} else {
		return false;
	}
}

static void doTest(int flags, const char* pattern, const char* const* expectedElements, int expectedElementsCount) {
	glob_t globInstance;
	int result = glob(pattern, flags, NULL, &globInstance);
	assert(result == 0);

	assert(areEqual(expectedElementsCount, expectedElements,
		globInstance.gl_pathc, globInstance.gl_pathv));

	globfree(&globInstance);
}


static void test1() {
	int flags = GLOB_ERR;
	const char* pattern = "*/[ACDP]/*[p]*.fru";

	const char* const expectedElements[] = {
		"Fruits/A/Apple.fru",
		"Fruits/A/Apricots.fru",
		"Fruits/C/Cantaloupe.fru",
		"Fruits/C/Custard-Apple.fru",
		"Fruits/P/Papaya.fru",
		"Fruits/P/Pineapple.fru"
	};

	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test2() {
	int flags = GLOB_ERR;
	const char* pattern = "*/[TIND]/*[p][r]*";

	const char* const expectedElements[] = {
		"Colors/D/Dark spring green.col"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test3() {
	int flags = GLOB_ERR;
	const char* pattern = "*/*/*ra*.cit";

	const char* const expectedElements[] = {
		"Cities/A/Ankara.cit",
		"Cities/B/Brasilia.cit",
		"Cities/G/Guadalajara.cit",
		"Cities/H/Hyderabad.cit",
		"Cities/K/Karachi.cit",
		"Cities/S/Surat.cit",
		"Cities/T/Tehran.cit"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test4() {
	int flags = GLOB_ERR;
	const char* pattern = "*/[!A]/[sS]*br*[n].[ac]*";

	const char* const expectedElements[] = {
		"Colors/S/Saddle brown.col",
		"Colors/S/Sandy brown.col",
		"Colors/S/Seal brown.col"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test5() {
	int flags = GLOB_ERR;
	const char* pattern = "*/[![:digit:]A-M]/*a*[[:punct:]][MF]*.*";

	const char* const expectedElements[] = {
		"Colors/R/Rich black (FOGRA29).col",
		"Colors/R/Rich black (FOGRA39).col"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test6() {
	int flags = GLOB_ERR;
	const char* pattern = "*/*/*[!0-9A-Za-z[:space:]\\-()]*.*";

	const char* const expectedElements[] = {
		"Colors/B/B'dazzled blue.col",
		"Colors/B/Big dip o'ruby.col",
		"Colors/C/Carmine (M&P).col",
		"Colors/D/Davy's grey.col",
		"Colors/H/Hooker's green.col",
		"Colors/O/Olive Drab #7.col",
		"Colors/O/Olive Drab (#3).col",
		"Colors/P/Payne's grey.col",
		"Colors/S/Screamin' Green.col",
		"Colors/S/St. Patrick's blue.col",
		"Fruits/S/Sapote, Mamey.fru"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test7() {
	int flags = GLOB_ERR;
	const char* pattern = "*/[RW]/*.fru";

	const char* const expectedElements[] = {
		"Fruits/R/Raspberries.fru",
		"Fruits/R/Rhubarb.fru",
		"Fruits/R/Rose-Apple.fru",
		"Fruits/W/Watermelon.fru"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test8() {
	int flags = GLOB_ERR;
	const char* pattern = "*/[VRW]/*.cou";

	const char* const expectedElements[] = {
		"Countries/R/Republic of the Congo.cou",
		"Countries/R/Reunion.cou",
		"Countries/R/Romania.cou",
		"Countries/R/Russia.cou",
		"Countries/R/Rwanda.cou",
		"Countries/V/Vanuatu.cou",
		"Countries/V/Vatican City.cou",
		"Countries/V/Venezuela.cou",
		"Countries/V/Vietnam.cou",
		"Countries/W/Wales.cou",
		"Countries/W/Western Sahara.cou"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test9() {
	int flags = GLOB_ERR;
	const char* pattern = "*ni*/[CD]/[DC][a-z][[:lower:]].a*";

	const char* const expectedElements[] = {
		"Animals/C/Cat.ani",
		"Animals/D/Dog.ani"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test10() {
	int flags = GLOB_ERR;
	const char* pattern = "C*/[![:digit:]]/[ADIKT]*[[:punct:]]*.c*";

	const char* const expectedElements[] = {
		"Colors/A/Azure (X11 - web color).col",
		"Colors/D/Dark green (X11).col",
		"Colors/D/Dark liver (horses).col",
		"Colors/D/Davy's grey.col",
		"Colors/I/International orange (Golden Gate Bridge).col",
		"Colors/I/International orange (aerospace).col",
		"Colors/I/International orange (engineering).col",
		"Colors/K/Khaki (X11) (Light khaki).col",
		"Colors/K/Khaki (web).col",
		"Colors/T/Tan (Crayola).col",
		"Colors/T/Tenne (tawny).col"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test11() {
	int flags = GLOB_ERR;
	const char* pattern = "F[:graph:]*/[[:xdigit:]]/*[[:blank:]]*.*";

	const char* const expectedElements[] = {
		"Fruits/A/Acerola - West Indian Cherry.fru",
		"Fruits/C/Coconut Meat.fru",
		"Fruits/D/Date Fruit.fru"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

static void test12() {
	int flags = GLOB_ERR | GLOB_MARK;
	const char* pattern = "F[:graph:]*/[[:xdigit:]]";

	const char* const expectedElements[] = {
		"Fruits/A/",
		"Fruits/B/",
		"Fruits/C/",
		"Fruits/D/",
		"Fruits/E/",
		"Fruits/F/"
	};
	int expectedElementsCount = sizeof(expectedElements) / sizeof(const char*);

	doTest(flags, pattern, expectedElements, expectedElementsCount);
}

int main(int argc, char** argv) {
	char* fileName = integrationTestCreateTemporaryFileName(argv[0]);
	assert(fileName != NULL);

	if (mkdir(fileName, S_IRWXU) != 0) {
		assert(errno == EEXIST);
	}
	chdir(fileName);

	struct GroupList* groupList = readGroups("/resources/groups.txt");
	createDirectories(groupList);

	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();
	test8();
	test9();
	test10();
	test11();
	test12();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
