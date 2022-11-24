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

#include "util/b_tree.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int availableMemoryBlocks = INT_MAX;
static void* memoryAllocatorAcquire(void* context, size_t size) {
	if (availableMemoryBlocks > 0) {
		availableMemoryBlocks--;
		return malloc(size);
	} else {
		return NULL;
	}
}

static void memoryAllocatorRelease(void* context, void* pointer) {
	free(pointer);
}

#define KEY_LENGTH 32
struct Element {
	char key[KEY_LENGTH];
	int64_t value1;
};

static int compare(const struct Element* element1, const struct Element* element2) {
	return strcmp(element1->key, element2->key);
}

/* Debug: */
// static bool printElement(void* unused, void* bTreeNodeId, struct Element* element) {
// 	printf("%p %p key=%s, value1=%ld\n", bTreeNodeId, element->key, element->key, element->value1);
// 	return false;
// }

// static void printBTreeElements(struct BTree* tree) {
// 	printf("The B-tree instance contains %d elements and %d nodes.\n", tree->elementCount, tree->nodeCount);
// 	bTreeForEach(tree, (bool (*)(void*, void*, void*)) &printElement, NULL);
// }

// static bool beforeNode(void* unused, uint32_t level) {
// 	printf("%u\n", level);
// 	return false;
// }

// static void printBTreeElementsByNode(struct BTree* tree) {
// 	printf("The B-tree instance contains %d elements and %d nodes.\n", tree->elementCount, tree->nodeCount);
// 	bTreeForEachNode(tree, (bool (*)(void*, void*, void*)) &printElement, &beforeNode, NULL, NULL);
// }

struct IsBTreeSortedContext {
	bool isSorted;
	struct Element* lastElement;
};

static bool isBTreeSortedStep(struct IsBTreeSortedContext* isBTreeSortedContext, void* bTreeNodeId, struct Element* element) {
	assert(isBTreeSortedContext->isSorted);
	assert(strlen(element->key) == element->value1);

	if (isBTreeSortedContext->lastElement != NULL) {
		isBTreeSortedContext->isSorted = compare(isBTreeSortedContext->lastElement, element) <= -1;
		isBTreeSortedContext->lastElement = element;
	}
	return isBTreeSortedContext->isSorted;
}

struct BTreeCountElementsContext {
	int count;
};

static bool bTreeCountElementsStep(struct BTreeCountElementsContext* bTreeCountElementsContext, void* bTreeNodeId, struct Element* element) {
	assert(strlen(element->key) == element->value1);
	bTreeCountElementsContext->count++;
	return false;
}

static int bTreeCountElements(struct BTree* tree) {
	struct BTreeCountElementsContext bTreeCountElementsContext;
	bTreeCountElementsContext.count = 0;
	bTreeForEach(tree, (bool (*)(void*, void*, void*)) &bTreeCountElementsStep, &bTreeCountElementsContext);
	return bTreeCountElementsContext.count;
}

static bool isBTreeSorted(struct BTree* tree) {
	struct IsBTreeSortedContext isBTreeSortedContext;
	isBTreeSortedContext.isSorted = true;
	isBTreeSortedContext.lastElement = NULL;
	bTreeForEach(tree, (bool (*)(void*, void*, void*)) &isBTreeSortedStep, &isBTreeSortedContext);
	return isBTreeSortedContext.isSorted;
}

static void testBTreeInsert(void) {
	struct BTree tree;

	bTreeInitialize(&tree, 256, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);

	struct Element element;

	strcpy(element.key, "house");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "cake");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "zoo");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "future");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "island");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "opposite");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "scissors");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "town");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "train");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "clock");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "banana");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "apple");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "lion");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "frog");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "rabbit");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "boy");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "wind");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "fire");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "airplane");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "water");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "ice");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "hour");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "waste");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "house");
	element.value1 = 1000;
	assert(B_TREE_KEY_ALREADY_EXISTS == bTreeInsert(&tree, &element));
	assert(B_TREE_SUCCESS == bTreeSearch(&tree, &element));
	assert(strcmp(element.key, "house") == 0);
	assert(element.value1 == strlen("house"));

	assert(bTreeSize(&tree) == 23);
	assert(isBTreeSorted(&tree));

	//printBTreeElements(&bTree);

	bTreeClear(&tree);
}

static void testBTreeSearch(void) {
	struct BTree tree;

	bTreeInitialize(&tree, 256, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(bTreeSize(&tree) == 0);

	struct Element element;

	strcpy(element.key, "house");
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	strcpy(element.key, "house");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));
	assert(bTreeSize(&tree) == 1);

	element.value1 = 1000;
	assert(B_TREE_SUCCESS == bTreeSearch(&tree, &element));
	assert(element.value1 == strlen(element.key));

	strcpy(element.key, "cow");
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	bTreeClear(&tree);
}

static void testBTreeRemove1(void) {
	struct BTree tree;

	bTreeInitialize(&tree, 320, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(bTreeSize(&tree) == 0);

	struct Element element;

	strcpy(element.key, "house");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "cake");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "zoo");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "future");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "island");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "opposite");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "scissors");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "town");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "train");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "fire");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "airplane");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "water");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "ice");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "hour");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "waste");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	assert(bTreeSize(&tree) == 15);
	assert(bTreeCountElements(&tree) == 15);
	assert(isBTreeSorted(&tree));

	//printBTreeElements(&bTree);

	strcpy(element.key, "opposite");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	assert(isBTreeSorted(&tree));

	strcpy(element.key, "house");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	assert(isBTreeSorted(&tree));

	strcpy(element.key, "ice");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	assert(isBTreeSorted(&tree));

	strcpy(element.key, "airplane");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "airplane");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOTHING_FOUND == bTreeRemove(&tree, &element));

	assert(bTreeSize(&tree) == 11);
	assert(bTreeCountElements(&tree) == 11);
	assert(isBTreeSorted(&tree));

	//printBTreeElements(&bTree);

	strcpy(element.key, "water");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "waste");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "train");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "town");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "train");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOTHING_FOUND == bTreeRemove(&tree, &element));

	assert(bTreeSize(&tree) == 7);
	assert(bTreeCountElements(&tree) == 7);
	assert(isBTreeSorted(&tree));

	//printBTreeElements(&bTree);

	strcpy(element.key, "zoo");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "scissors");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "future");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "cake");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "train");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	assert(isBTreeSorted(&tree));
	assert(bTreeSize(&tree) == 4);
	assert(bTreeCountElements(&tree) == 4);

	//printBTreeElements(&bTree);

	strcpy(element.key, "train");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "island");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "hour");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	strcpy(element.key, "fire");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	assert(isBTreeSorted(&tree));
	assert(bTreeSize(&tree) == 0);
	assert(bTreeCountElements(&tree) == 0);

	//printBTreeElements(&bTree);

	strcpy(element.key, "hour");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "fire");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "hour");
	assert(B_TREE_SUCCESS == bTreeSearch(&tree, &element));
	assert(element.value1 == strlen(element.key));

	strcpy(element.key, "fire");
	assert(B_TREE_SUCCESS == bTreeSearch(&tree, &element));
	assert(element.value1 == strlen(element.key));

	strcpy(element.key, "fire");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));

	bTreeClear(&tree);
	assert(bTreeCountElements(&tree) == bTreeSize(&tree));
}

static void testBTreeRemove2(void) {
	const int BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];

	struct BTree tree;
	bTreeInitialize(&tree, 512, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);

	struct Element element;

	const int MAX_UNIQUE_WORD_COUNT = 5000;
	char** words = malloc(sizeof(char*) * MAX_UNIQUE_WORD_COUNT);
	int uniqueWordCount = 0;

	/* Insert some elements. */
	FILE* file = fopen("../resources/test/unit/words.txt", "r");
	if (file != NULL) {
		while (!feof(file)) {
			if (NULL != fgets(buffer, BUFFER_SIZE, file)) {
				size_t length = strlen(buffer);
				if (length > 0) {
					if (buffer[length - 1] == '\n') {
						buffer[length - 1] = '\0';
						length--;
					}
					if (strlen(buffer) < KEY_LENGTH) {
						strcpy(element.key, buffer);
						if (B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element)) {
							words[uniqueWordCount] = malloc((length + 1) * sizeof(char));
							strcpy(words[uniqueWordCount], buffer);

							element.value1 = length;
							assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));
							uniqueWordCount++;
							assert(bTreeCountElements(&tree) == uniqueWordCount);
							assert(MAX_UNIQUE_WORD_COUNT >= uniqueWordCount);
						}
					}
				}
			}
		}
		fclose(file);

	} else {
		perror(NULL);
		assert(false);
	}

	assert(isBTreeSorted(&tree));
	assert(bTreeSize(&tree) == uniqueWordCount);
	assert(bTreeCountElements(&tree) == uniqueWordCount);

	//printBTreeElements(&tree);

	for (int i = uniqueWordCount - 1; i >= 0; i--) {
		const char* word = words[i];

		strcpy(element.key, word);

		assert(B_TREE_SUCCESS == bTreeSearch(&tree, &element));
	}

	assert(isBTreeSorted(&tree));
	assert(bTreeSize(&tree) == uniqueWordCount);
	assert(bTreeCountElements(&tree) == uniqueWordCount);

	for (int i = uniqueWordCount - 1; i >= 0; i--) {
		const char* word = words[i];

		assert (bTreeCountElements(&tree) == bTreeSize(&tree));

		strcpy(element.key, word);
		assert(B_TREE_SUCCESS == bTreeRemove(&tree, &element));
	}

	for (int i = 0; i < uniqueWordCount; i++) {
		free(words[i]);
	}
	free(words);

	bTreeClear(&tree);
	assert(bTreeCountElements(&tree) == bTreeSize(&tree));
}

static void testBTreeInsertWithLimitedMemory1(void) {
	struct BTree tree;

	bTreeInitialize(&tree, 215, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(tree.order == 5);

	struct Element element;

	strcpy(element.key, "a");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "g");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "f");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "b");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "k");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "d");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "h");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "m");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "j");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "e");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "s");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "i");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "r");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "x");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "c");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "l");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "n");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "t");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "u");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "p");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 0;

	strcpy(element.key, "y");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = INT_MAX;

	strcpy(element.key, "y01");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y02");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y04");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y05");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y06");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y07");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 1;

	strcpy(element.key, "y08");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 1;

	strcpy(element.key, "y03");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = INT_MAX;

	strcpy(element.key, "y09");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y10");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y11");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y12");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y13");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y14");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y15");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y16");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y17");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y18");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y19");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y20");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y21");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y22");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y23");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y24");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y25");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y26");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y27");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y28");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y29");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y30");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y31");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y32");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y33");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y34");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y35");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 2;

	strcpy(element.key, "y36");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 3;

	strcpy(element.key, "y36");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	bTreeClear(&tree);
}

static void testBTreeInsertWithLimitedMemory2(void) {
	struct BTree tree;

	bTreeInitialize(&tree, 215, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(tree.order == 5);

	struct Element element;

	availableMemoryBlocks = INT_MAX;

	strcpy(element.key, "z");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "y");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "x");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "w");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "v");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "u");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "t");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "s");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "r");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "q");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "p");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "o");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "n");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "m");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "l");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "k");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 2;

	strcpy(element.key, "j");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = INT_MAX;

	strcpy(element.key, "j");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "i");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "h");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "g");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "f");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "e");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "d");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "c");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "b");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9999");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9998");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9997");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9996");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9995");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9994");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9993");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9992");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9991");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9989");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9988");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9987");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9986");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9985");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9984");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9983");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9982");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9981");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9980");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9979");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9978");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9977");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9976");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9975");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9974");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	strcpy(element.key, "a9973");
	element.value1 = strlen(element.key);
	assert(B_TREE_SUCCESS == bTreeInsert(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	availableMemoryBlocks = 3;

	strcpy(element.key, "a9972");
	element.value1 = strlen(element.key);
	assert(B_TREE_NOT_ENOUGH_MEMORY == bTreeInsert(&tree, &element));
	assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));

	//printBTreeElementsByNode(&tree);
	assert(isBTreeSorted(&tree));

	bTreeClear(&tree);
}

static void testBTreeInsertWithLimitedMemory3(void) {
	struct BTree tree;

	const int MAX_UNIQUE_WORD_COUNT = 100000;
	char** words = malloc(sizeof(char*) * MAX_UNIQUE_WORD_COUNT);
	int uniqueWordCount = 0;

	bTreeInitialize(&tree, 215, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(tree.order == 5);

    srand(1);

	while (uniqueWordCount < MAX_UNIQUE_WORD_COUNT) {
		char string[KEY_LENGTH];
		for (int i = 0; i < KEY_LENGTH - 1; i++) {
			string[i] = 'a' + rand() % 26;
		}
		string[KEY_LENGTH - 1] = '\0';

		struct Element element;
		strcpy(element.key, string);
		element.value1 = strlen(element.key);

		int operation = rand() % 5;
		if (operation == 0) {
			availableMemoryBlocks = INT_MAX;
		} else {
			availableMemoryBlocks = operation;
		}
		//printf("I will try to insert '%s'. I have %d available blocks.\n", string, availableMemoryBlocks);

		enum OperationResult result = bTreeInsert(&tree, &element);
		if (result == B_TREE_SUCCESS) {
			words[uniqueWordCount] = malloc(sizeof(char) * KEY_LENGTH);
			strcpy(words[uniqueWordCount], string);
			uniqueWordCount++;

		} else if (result == B_TREE_NOT_ENOUGH_MEMORY) {
			assert(B_TREE_NOTHING_FOUND == bTreeSearch(&tree, &element));;
		}

		//printBTreeElementsByNode(&tree);
	}

	assert(isBTreeSorted(&tree));

	for (int i = 0; i < uniqueWordCount; i++) {
		struct Element element;
		strcpy(element.key, words[i]);

		assert(bTreeSearch(&tree, &element) == B_TREE_SUCCESS);
		assert(element.value1 == strlen(element.key));

		free(words[i]);
	}
	free(words);

	bTreeClear(&tree);
}

static void testBTreeWorstCaseNodeCountToStore1(void) {
	struct BTree tree;

	bTreeInitialize(&tree, 215, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(tree.order == 5);
	assert(bTreeWorstCaseNodeCountToStore(215, sizeof(struct Element), 434) == 437);

	bTreeInitialize(&tree, 542, sizeof(struct Element),
			NULL, &memoryAllocatorAcquire, &memoryAllocatorRelease,
			(int (*)(const void*, const void*)) &compare);
	assert(tree.order == 13);
	assert(bTreeWorstCaseNodeCountToStore(542, sizeof(struct Element), 50000) == 10004);
}

int main(int argc, char** argv) {
	testBTreeInsert();
	testBTreeSearch();
	testBTreeRemove1();
	testBTreeRemove2();
	testBTreeInsertWithLimitedMemory1();
	testBTreeInsertWithLimitedMemory2();
	testBTreeInsertWithLimitedMemory3();
	testBTreeWorstCaseNodeCountToStore1();

	return 0;
}
