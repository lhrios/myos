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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "user/util/checksum.h"

#include "util/math_utils.h"

#include "test/integration_test.h"

struct Node;
struct Node {
	struct Node* leftNode;
	struct Node* rightNode;
	char* key;
	int count;
	uint16_t nodeChecksum;
	uint16_t keyChecksum;
};

static uint16_t calculateNodeChecksum(struct Node* node) {
	return fletcher16(node, sizeof(struct Node) - 2 * sizeof(uint16_t));
}

static uint16_t calculateKeyChecksum(char* key) {
	return fletcher16(key, strlen(key) + 1);
}

static void updateChecksums(struct Node* node) {
	node->nodeChecksum = calculateNodeChecksum(node);
	node->keyChecksum = calculateKeyChecksum(node->key);
}

static bool doChecksumsMatch(struct Node* node) {
	return node->keyChecksum == calculateKeyChecksum(node->key) && node->nodeChecksum == calculateNodeChecksum(node);
}

static struct Node* createNode(char* key) {
	struct Node* node = malloc(sizeof(struct Node));
	assert(node != NULL);
	node->leftNode = NULL;
	node->rightNode = NULL;
	node->key = key;
	node->count = 1;
	updateChecksums(node);
	return node;
}

static void insertRecursive(struct Node* node, char* key) {
	assert(doChecksumsMatch(node));

	int result = strcmp(node->key, key);
	if (result < 0) {
		if (node->leftNode != NULL) {
			insertRecursive(node->leftNode, key);

		} else {
			node->leftNode = createNode(key);
			updateChecksums(node);
		}

	} else if (result > 0) {
		if (node->rightNode != NULL) {
			insertRecursive(node->rightNode, key);

		} else {
			node->rightNode = createNode(key);
			updateChecksums(node);
		}

	} else {
		node->count++;
		updateChecksums(node);
	}
}

static void insert(struct Node** root, char* key) {
	char* keyCopy = strdup(key);
	assert(keyCopy != NULL);
	assert(strcmp(key, keyCopy) == 0);

	if (*root == NULL) {
		*root = createNode(keyCopy);
	} else {
		insertRecursive(*root, keyCopy);
	}
}

static struct Node* search(struct Node* root, char* key) {
	struct Node* currentNode = root;

	while (currentNode != NULL) {
		int result = strcmp(currentNode->key, key);
		if (result < 0) {
			currentNode = currentNode->leftNode;

		} else if (result > 0) {
			currentNode = currentNode->rightNode;

		} else {
			return currentNode;
		}
	}

	return NULL;
}

static void assertCountIsZero(struct Node* node) {
	assert(node->count == 0);
}

const char* const PREFIX = "________________PREFIX________________";

static void appendPrefix(struct Node* node) {
	size_t length = strlen(node->key);
	char* key = realloc(node->key, strlen(PREFIX) + length + 1);
	assert(key != NULL);
	memmove(key + strlen(PREFIX), key, length + 1);
	memcpy(key, PREFIX, strlen(PREFIX));
	node->key = key;
	updateChecksums(node);
}

static void freeNodeAndKey(struct Node* node) {
	free(node->key);
	free(node);
}

static void inOrderWalk(struct Node* node, void (*visit)(struct Node*)) {
	assert(doChecksumsMatch(node));
	if (node->leftNode != NULL) {
		inOrderWalk(node->leftNode, visit);
	}
	visit(node);
	if (node->rightNode != NULL) {
		inOrderWalk(node->rightNode, visit);
	}
}

static void postOrderWalk(struct Node* node, void (*visit)(struct Node*)) {
	assert(doChecksumsMatch(node));
	if (node->leftNode != NULL) {
		postOrderWalk(node->leftNode, visit);
	}
	if (node->rightNode != NULL) {
		postOrderWalk(node->rightNode, visit);
	}
	visit(node);
}

static void testMalloc(void) {
	struct Node* root = NULL;

	FILE* file = fopen("/resources/words.txt", "r");
	assert(file != NULL);

	const size_t bufferSize = 1024 * sizeof(char);
	char* buffer = malloc(bufferSize);
	assert(buffer != NULL);

	/* Part 1: */
	while (true) {
		char* key = fgets(buffer, bufferSize, file);
		if (key == NULL) {
			break;
		} else {
			insert(&root, buffer);
		}
	}
	assert(feof(file));

	/* Part 2: */
	rewind(file);
	assert(!feof(file));

	while (true) {
		char* key = fgets(buffer, bufferSize, file);
		if (key == NULL) {
			break;
		} else {
			struct Node* node = search(root, buffer);
			assert(node != NULL);
			assert(node->count > 0);
			node->count--;
			updateChecksums(node);
		}
	}
	assert(feof(file));

	/* Part 3: */
	inOrderWalk(root, &assertCountIsZero);

	/* Part 4: */
	inOrderWalk(root, &appendPrefix);

	/* Part 5: */
	rewind(file);
	assert(!feof(file));

	size_t prefixLength = strlen(PREFIX);
	strcpy(buffer, PREFIX);
	while (true) {
		char* key = fgets(buffer + prefixLength, bufferSize - prefixLength, file);
		if (key == NULL) {
			break;
		} else {
			struct Node* node = search(root, buffer);
			assert(node != NULL);
			node->count++;
			updateChecksums(node);
		}
	}
	assert(feof(file));

	/* Part 6: */
	rewind(file);
	assert(!feof(file));

	while (true) {
		char* key = fgets(buffer + prefixLength, bufferSize - prefixLength, file);
		if (key == NULL) {
			break;
		} else {
			struct Node* node = search(root, buffer);
			assert(node != NULL);
			assert(node->count > 0);
			node->count--;
			updateChecksums(node);
		}
	}
	assert(feof(file));

	/* Part 7: */
	inOrderWalk(root, &assertCountIsZero);

	/* Part 8: */
	postOrderWalk(root, &freeNodeAndKey);

	fclose(file);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	testMalloc();

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
