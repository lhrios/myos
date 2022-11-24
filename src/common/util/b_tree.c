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
#include <string.h>

#include "util/b_tree.h"
#include "util/math_utils.h"
#include "util/search_utils.h"

/*
 * References:
 * - Data Structures and Program Design in C++
 */

struct BTreeNode {
   uint32_t id;
   size_t elementsCount;
	struct BTreeNode* children[];
} __attribute__((packed));

static bool isLeafNode(struct BTreeNode* node) {
	for (int i = 0; i < node->elementsCount; i++) {
		if (node->children[i] != NULL) {
			return false;
		}
	}

	return true;
}

static void* getElementAddress(struct BTree* tree, struct BTreeNode* node, int index) {
	return (((void*) &node->children) + sizeof(struct BTreeNode*) * tree->order) + (index * tree->elementSize);
}

static struct BTreeNode* newNode(struct BTree* tree) {
	struct BTreeNode* node = tree->memoryAllocatorAcquire(tree->memoryAllocatorContext, tree->treeNodeSize);
	if (node != NULL) {
		node->elementsCount = 0;
		memset(&node->children, 0, tree->order * sizeof(struct BTreeNode*));
		node->id = tree->nodeCount++;
	}
	return node;
}

static enum OperationResult searchInsideNode(struct BTree* tree, struct BTreeNode* node, void* elementToSearch, int* index) {
	const void* array = getElementAddress(tree, node, 0);
	int result = searchUtilsBinarySearchComparingElements(elementToSearch, array, node->elementsCount, tree->elementSize, tree->elementKeyComparator);
	if (result >= 0) {
		*index = result;
		return B_TREE_SUCCESS;
	} else {
		*index = -result - 1;
		return B_TREE_NOTHING_FOUND;
	}
}

static enum OperationResult recursiveSearch(struct BTree* tree, struct BTreeNode* node, void* elementToSearch) {
	enum OperationResult operationResult = B_TREE_NOTHING_FOUND;
	if (node != NULL) {
		int index;
		operationResult = searchInsideNode(tree, node, elementToSearch, &index);
		if (operationResult == B_TREE_NOTHING_FOUND) {
			operationResult = recursiveSearch(tree, node->children[index], elementToSearch);
		} else {
			memcpy(elementToSearch, getElementAddress(tree, node, index), tree->elementSize);
		}
	}

	return operationResult;
}

static void insertInsideNode(struct BTree* tree, struct BTreeNode* node, void* elementToInsert, struct BTreeNode* rightBTreeNode, int index) {
	assert(node->elementsCount + 1 < tree->order);

	memmove(getElementAddress(tree, node, index + 1), getElementAddress(tree, node, index), tree->elementSize * (node->elementsCount - index));
	memmove(&node->children[index + 1 + 1], &node->children[index + 1], sizeof(struct BTreeNode*) * (node->elementsCount - index));

	node->children[index + 1] = rightBTreeNode;
	memcpy(getElementAddress(tree, node, index), elementToInsert, tree->elementSize);
	node->elementsCount++;
}

static enum OperationResult splitNode(struct BTree* tree, struct BTreeNode* node, void* elementToInsert, struct BTreeNode* rightBTreeNodeToInsert,
		int index, void** overflowedElement, struct BTreeNode** overflowedRightBTreeNode) {
	assert(node->elementsCount + 1 == tree->order);
	*overflowedRightBTreeNode = newNode(tree);

	if (*overflowedRightBTreeNode != NULL) {
		int middle = tree->order / 2;

		/* Does the new element belong to the node that already exists? */
		bool belongsToLeft;
		if (index <= middle) {
			belongsToLeft = true;

		} else {
			middle++;
			belongsToLeft = false;
		}

		memcpy(getElementAddress(tree, *overflowedRightBTreeNode, 0), getElementAddress(tree, node, middle), tree->elementSize * (tree->order - 1 - middle));
		memcpy(&(*overflowedRightBTreeNode)->children[1], &node->children[middle + 1], sizeof(struct BTreeNode*) * (tree->order - 1 - middle));
		node->elementsCount = middle;
		(*overflowedRightBTreeNode)->elementsCount = tree->order - 1 - middle;

		if (belongsToLeft) {
			insertInsideNode(tree, node, elementToInsert, rightBTreeNodeToInsert, index);
		} else {
			insertInsideNode(tree, *overflowedRightBTreeNode, elementToInsert, rightBTreeNodeToInsert, index - middle);
		}

		*overflowedElement = getElementAddress(tree, node, node->elementsCount - 1);
		(*overflowedRightBTreeNode)->children[0] = node->children[node->elementsCount];
		node->elementsCount--;

		return B_TREE_SUCCESS;

	} else {
		return B_TREE_NOT_ENOUGH_MEMORY;
	}
}

static void removeFromLeafNode(struct BTree* tree, struct BTreeNode* node, int index) {
	memmove(getElementAddress(tree, node, index), getElementAddress(tree, node, index + 1), tree->elementSize * (node->elementsCount - 1 - index));
	node->elementsCount--;
}

static int getNewestNodeIndex(struct BTree* tree, struct BTreeNode* node1, struct BTreeNode* node2) {
	int newestNodeIndex = 0;
	struct BTreeNode* newestNode = node1->children[0];
	for (int i = 1; i < node1->elementsCount + 1 + node2->elementsCount + 1; i++) {
		struct BTreeNode* node;

		if (i < node1->elementsCount + 1) {
			node = node1->children[i];
		} else {
			assert(i - (node1->elementsCount + 1) < node2->elementsCount + 1);
			node = node2->children[i - (node1->elementsCount + 1)];
		}
		assert(node != NULL);

		if (newestNode->id < node->id) {
			newestNodeIndex = i;
			newestNode = node;
		}
	}

	return newestNodeIndex;
}

static void undoPushDown(struct BTree* tree, struct BTreeNode* node, void* elementToInsert, void* overflowedElement, struct BTreeNode* overflowedRightBTreeNode) {
	/*
	 * Note that "node" and "overflowedRightBTreeNode" belong to the same tree level.
	 * The splitting of "node" produced "overflowedRightBTreeNode" and "overflowedElement" has been overflowed to "node"'s parent.
	 * The variable "elementToInsert" represents the element that was originally inserted.
	 */

	if (node != NULL) {
		assert(overflowedRightBTreeNode != NULL);

		if (node->children[0] == NULL) {
			/* Both nodes are leafs, so the recursion stops. */

			assert(isLeafNode(node));
			assert(isLeafNode(overflowedRightBTreeNode));

			int index1;
			int index2;
			enum OperationResult searchInsideNodeResult1 = searchInsideNode(tree, node, elementToInsert, &index1);
			enum OperationResult searchInsideNodeResult2 = searchInsideNode(tree, overflowedRightBTreeNode, elementToInsert, &index2);

			bool insertOverflowedElement = true;
			if (B_TREE_SUCCESS == searchInsideNodeResult1) {
				removeFromLeafNode(tree, node, index1);
			} else if (B_TREE_SUCCESS == searchInsideNodeResult2) {
				removeFromLeafNode(tree, overflowedRightBTreeNode, index2);
			} else {
				assert(tree->elementKeyComparator(elementToInsert, overflowedElement) == 0);
				insertOverflowedElement = false;
			}

			if (insertOverflowedElement) {
				memmove(getElementAddress(tree, node, node->elementsCount), overflowedElement, tree->elementSize);
				node->elementsCount++;
			}

			for (int i = 0; i < overflowedRightBTreeNode->elementsCount; i++) {
				memcpy(getElementAddress(tree, node, node->elementsCount + i), getElementAddress(tree, overflowedRightBTreeNode, i), tree->elementSize);
			}
			node->elementsCount += overflowedRightBTreeNode->elementsCount;

		} else {
			assert(!isLeafNode(node));
			assert(!isLeafNode(overflowedRightBTreeNode));

			int index = getNewestNodeIndex(tree, node, overflowedRightBTreeNode) - 1;
			assert(index >= 0);

			if (index < node->elementsCount) {
				/* The overflowed node (that came from a push down operation on node's children) was inserted on the first node. */

				void* newOverflowedElement = getElementAddress(tree, node, index);
				struct BTreeNode* newOverflowedRightBTreeNode = node->children[index + 1];

				undoPushDown(tree,node->children[index], elementToInsert, newOverflowedElement, newOverflowedRightBTreeNode);

				for (int i = index; i < node->elementsCount - 1; i++) {
					memcpy(getElementAddress(tree, node, i), getElementAddress(tree, node, i + 1), tree->elementSize);
					node->children[i + 1] = node->children[i + 2];
				}
				node->elementsCount--;

				/* Reinsert the median entry that has been overflowed during the node splitting operation. */
				memcpy(getElementAddress(tree, node, node->elementsCount), overflowedElement, tree->elementSize);
				node->children[node->elementsCount + 1] = overflowedRightBTreeNode->children[0];
				node->elementsCount++;

				for (int i = 0; i < overflowedRightBTreeNode->elementsCount; i++) {
					memcpy(getElementAddress(tree, node, node->elementsCount), getElementAddress(tree, overflowedRightBTreeNode, i), tree->elementSize);
					node->children[node->elementsCount + 1] = overflowedRightBTreeNode->children[i + 1];
					node->elementsCount++;
				}

			} else if (index < node->elementsCount + 1) {
				/* The overflowed node (that came from a push down operation on node's children) has been overflowed again (to its parent). */

				void* newOverflowedElement = overflowedElement;
				struct BTreeNode* newOverflowedRightBTreeNode = overflowedRightBTreeNode->children[0];

				undoPushDown(tree, node->children[node->elementsCount], elementToInsert, newOverflowedElement, newOverflowedRightBTreeNode);

				for (int i = 0; i < overflowedRightBTreeNode->elementsCount; i++) {
					memcpy(getElementAddress(tree, node, node->elementsCount), getElementAddress(tree, overflowedRightBTreeNode, i), tree->elementSize);
					node->children[node->elementsCount + 1] = overflowedRightBTreeNode->children[i + 1];
					node->elementsCount++;
				}

			} else {
				/* The overflowed node (that came from a push down operation on node's children) was inserted on the second node. */

				index -= node->elementsCount + 1;
				assert(0 <= index && index < overflowedRightBTreeNode->elementsCount);

				void* newOverflowedElement = getElementAddress(tree, overflowedRightBTreeNode, index);
				struct BTreeNode* newOverflowedRightBTreeNode = overflowedRightBTreeNode->children[index + 1];

				undoPushDown(tree, overflowedRightBTreeNode->children[index], elementToInsert, newOverflowedElement, newOverflowedRightBTreeNode);

				/* Reinsert the median entry that has been overflowed during the node splitting operation. */
				memmove(getElementAddress(tree, node, node->elementsCount), overflowedElement, tree->elementSize);
				node->children[node->elementsCount + 1] = overflowedRightBTreeNode->children[0];
				node->elementsCount++;

				for (int i = 0; i < overflowedRightBTreeNode->elementsCount; i++) {
					if (i != index) {
						memcpy(getElementAddress(tree, node, node->elementsCount), getElementAddress(tree, overflowedRightBTreeNode, i), tree->elementSize);
						node->children[node->elementsCount + 1] = overflowedRightBTreeNode->children[i + 1];
						node->elementsCount++;
					} else {
						/* Do not insert the node that was not here originally (before the overflow in one of child). */
						assert(overflowedRightBTreeNode->children[i + 1] == newOverflowedRightBTreeNode);
					}
				}
			}
		}

		tree->memoryAllocatorRelease(tree->memoryAllocatorContext, overflowedRightBTreeNode);
		tree->nodeCount--;
	}
}

static enum OperationResult pushDown(struct BTree* tree, struct BTreeNode* node, void* elementToInsert, void** overflowedElement, struct BTreeNode** overflowedRightBTreeNode) {
	/* Down, here, means in direction of tree's leafs. */

	enum OperationResult operationResult;
	if (node == NULL) {
		*overflowedElement = elementToInsert;
		*overflowedRightBTreeNode = NULL;
		operationResult = B_TREE_OVERFLOW;

	} else {
		int index;
		if (B_TREE_SUCCESS == searchInsideNode(tree, node, elementToInsert, &index)) {
			operationResult = B_TREE_KEY_ALREADY_EXISTS;

		} else {
			 void* localOverflowedElement;
			 struct BTreeNode* localOverflowedRightBTreeNode = NULL;

			if (B_TREE_OVERFLOW == (operationResult = pushDown(tree, node->children[index], elementToInsert, &localOverflowedElement, &localOverflowedRightBTreeNode))) {
				/* Is there enough space left to handle child overflow? */
				if (node->elementsCount + 1 < tree->order) {
					insertInsideNode(tree, node, localOverflowedElement, localOverflowedRightBTreeNode, index);
					operationResult = B_TREE_SUCCESS;

				} else {
					if (B_TREE_SUCCESS == splitNode(tree, node, localOverflowedElement, localOverflowedRightBTreeNode, index, overflowedElement, overflowedRightBTreeNode)) {
						operationResult = B_TREE_OVERFLOW;

					} else {
						undoPushDown(tree, node->children[index], elementToInsert, localOverflowedElement, localOverflowedRightBTreeNode);
						operationResult = B_TREE_NOT_ENOUGH_MEMORY;
					}
				}
			}
		}
	}

	return operationResult;
}

enum OperationResult bTreeSearch(struct BTree* tree, void* elementToSearch) {
	return recursiveSearch(tree, tree->root, elementToSearch);
}

static int calculateOrder(size_t treeNodeSize, size_t elementSize) {
	return ((int) treeNodeSize - sizeof(struct BTreeNode) + (int) elementSize)
			/ (sizeof(void*) + (int) elementSize);
}

void bTreeInitialize(struct BTree* tree, size_t treeNodeSize, size_t elementSize,
		void* memoryAllocatorContext, void* (*memoryAllocatorAcquire)(void*, size_t), void (*memoryAllocatorRelease)(void*, void*),
		int (*elementKeyComparator)(const void*, const void*)) {

	int order = calculateOrder(treeNodeSize, elementSize);
	assert(order >= 3);
	assert(treeNodeSize >= sizeof(struct BTreeNode) + sizeof(void*) * order + elementSize * (order - 1));

	tree->treeNodeSize = treeNodeSize;
	tree->elementSize = elementSize;
	tree->order = order;
	tree->root = NULL;
	tree->memoryAllocatorContext = memoryAllocatorContext;
	tree->memoryAllocatorAcquire = memoryAllocatorAcquire;
	tree->memoryAllocatorRelease = memoryAllocatorRelease;
	tree->elementKeyComparator = elementKeyComparator;
	tree->elementCount = 0;
	tree->nodeCount = 0;
}

enum OperationResult bTreeInsert(struct BTree* tree, void* elementToInsert) {
	void* overflowedElement;
	struct BTreeNode* overflowedRightBTreeNode;

	enum OperationResult operationResult = pushDown(tree, tree->root, elementToInsert, &overflowedElement, &overflowedRightBTreeNode);
	if (B_TREE_OVERFLOW == operationResult) {
		struct BTreeNode* newRoot = newNode(tree);
		if (newRoot != NULL) {
			memcpy(getElementAddress(tree, newRoot, 0), overflowedElement, tree->elementSize);
			newRoot->elementsCount = 1;
			newRoot->children[0] = tree->root;
			newRoot->children[1] = overflowedRightBTreeNode;
			tree->root = newRoot;
			operationResult = B_TREE_SUCCESS;

		} else {
			undoPushDown(tree, tree->root, elementToInsert, overflowedElement, overflowedRightBTreeNode);
			operationResult = B_TREE_NOT_ENOUGH_MEMORY;
		}
	}

	if (operationResult == B_TREE_SUCCESS) {
		tree->elementCount++;
	}

	return operationResult;
}

static bool recursiveForEach(struct BTree* tree, struct BTreeNode* node, bool (*processElement)(void*, void*, void*), void* argument) {
	if (node->children[0] != NULL) {
		if (recursiveForEach(tree, node->children[0], processElement, argument)) {
			return true;
		}
	}
	for (int i = 0; i < node->elementsCount; i++) {
		if (processElement(argument, node, getElementAddress(tree, node, i))) {
			return true;
		}
		if (node->children[i + 1] != NULL) {
			if (recursiveForEach(tree, node->children[i + 1], processElement, argument)) {
				return true;
			}
		}
	}

	return false;
}

static bool recursiveForEachNode(struct BTree* tree, struct BTreeNode* node, uint32_t level, bool (*processElement)(void*, void*, void*), bool (*beforeNode)(void*, uint32_t), bool (*afterNode)(void*), void* argument) {
	if (beforeNode) {
		if (beforeNode(argument, level)) {
			return true;
		}
	}
	for (int i = 0; i < node->elementsCount; i++) {
		if (processElement(argument, node, getElementAddress(tree, node, i))) {
			return true;
		}
	}
	if (afterNode) {
		if (afterNode(argument)) {
			return true;
		}
	}

	for (int i = 0; i < node->elementsCount + 1; i++) {
		if (node->children[i] != NULL) {
			if (recursiveForEachNode(tree, node->children[i], level + 1, processElement, beforeNode, afterNode, argument)) {
				return true;
			}
		}
	}

	return false;
}

static void removeFromNonLeafNode(struct BTree* tree, struct BTreeNode* node, int index) {
	struct BTreeNode* leaf = node->children[index];
	assert(leaf != NULL);
	/* Go down the tree until find a node that is a leaf. */
	while (leaf->children[leaf->elementsCount] != NULL) {
		leaf = leaf->children[leaf->elementsCount];
	}
	memcpy(getElementAddress(tree, node, index), getElementAddress(tree, leaf, leaf->elementsCount - 1), tree->elementSize);
}

static void moveLefMostElement(struct BTree* tree, struct BTreeNode* node, int index) {
	struct BTreeNode* leftNode = node->children[index - 1];
	struct BTreeNode* rightNode = node->children[index];

	memcpy(getElementAddress(tree, leftNode, leftNode->elementsCount), getElementAddress(tree, node, index - 1), tree->elementSize);

	leftNode->elementsCount++;
	leftNode->children[leftNode->elementsCount] = rightNode->children[0];

	memcpy(getElementAddress(tree, node, index - 1), getElementAddress(tree, rightNode, 0), tree->elementSize);

	rightNode->elementsCount--;
	memmove(getElementAddress(tree, rightNode, 0), getElementAddress(tree, rightNode, 1), tree->elementSize * rightNode->elementsCount);
	memmove(&rightNode->children[0], &rightNode->children[1], sizeof(struct BTreeNode*) * rightNode->elementsCount);
	rightNode->children[rightNode->elementsCount] = rightNode->children[rightNode->elementsCount + 1];
}

static void moveRightMostElement(struct BTree* tree, struct BTreeNode* node, int index) {
	struct BTreeNode* leftNode = node->children[index];
	struct BTreeNode* rightNode = node->children[index + 1];

	rightNode->children[rightNode->elementsCount + 1] = rightNode->children[rightNode->elementsCount];

	memmove(getElementAddress(tree, rightNode, 1), getElementAddress(tree, rightNode, 0), tree->elementSize * rightNode->elementsCount);
	memmove(&rightNode->children[1], &rightNode->children[0], sizeof(struct BTreeNode*) * rightNode->elementsCount);
	rightNode->elementsCount++;
	memcpy(getElementAddress(tree, rightNode, 0), getElementAddress(tree, node, index), tree->elementSize);

	rightNode->children[0] = leftNode->children[leftNode->elementsCount--];
	memcpy(getElementAddress(tree, node, index), getElementAddress(tree, leftNode, leftNode->elementsCount), tree->elementSize);
}

static void combineLeftAndRight(struct BTree* tree, struct BTreeNode* node, int index) {
	struct BTreeNode* leftNode = node->children[index - 1];
	struct BTreeNode* rightNode = node->children[index];

	memcpy(getElementAddress(tree, leftNode, leftNode->elementsCount), getElementAddress(tree, node, index - 1), tree->elementSize);

	leftNode->elementsCount++;
	leftNode->children[leftNode->elementsCount] = rightNode->children[0];

	memcpy(getElementAddress(tree, leftNode, leftNode->elementsCount), getElementAddress(tree, rightNode, 0), tree->elementSize * rightNode->elementsCount);
	memcpy(&leftNode->children[leftNode->elementsCount + 1], &rightNode->children[1], sizeof(struct BTreeNode*) * rightNode->elementsCount);
	leftNode->elementsCount += rightNode->elementsCount;

	node->elementsCount--;
	memmove(getElementAddress(tree, node, index - 1), getElementAddress(tree, node, index), tree->elementSize * (node->elementsCount - index + 1));
	memmove(&node->children[index], &node->children[index + 1], sizeof(struct BTreeNode*) * (node->elementsCount - index + 1));


	tree->memoryAllocatorRelease(tree->memoryAllocatorContext, rightNode);
	tree->nodeCount--;
}

static void restoreBTreeInvariants(struct BTree* tree, struct BTreeNode* node, int index) {
	int minimumElementsPerNode = (tree->order - 1) / 2;

	/* Is it the last child? */
	if (index == node->elementsCount) {
		if (node->children[index - 1]->elementsCount > minimumElementsPerNode) {
			moveRightMostElement(tree, node, index - 1);
		} else {
			combineLeftAndRight(tree, node, index);
		}

	/* Is it the first one? */
	} else if (index == 0) {
		if (node->children[1]->elementsCount > minimumElementsPerNode) {
			moveLefMostElement(tree, node, 1);
		} else {
			combineLeftAndRight(tree, node, 1);
		}

	} else {
		if (node->children[index - 1]->elementsCount > minimumElementsPerNode) {
			moveRightMostElement(tree, node, index - 1);
		} else if (node->children[index + 1]->elementsCount > minimumElementsPerNode) {
			moveLefMostElement(tree, node, index + 1);
		} else {
			combineLeftAndRight(tree, node, index);
		}
	}
}

static enum OperationResult recursiveRemove(struct BTree* tree, struct BTreeNode* node, void* elementToRemove)  {
	enum OperationResult operationResult;
	int index;
	if (node != NULL) {
		if (B_TREE_SUCCESS == searchInsideNode(tree, node, elementToRemove, &index)) {
			operationResult = B_TREE_SUCCESS;
			/* Is it a leaf node? */
			if (node->children[index] == NULL) {
				removeFromLeafNode(tree, node, index);

			} else {
				removeFromNonLeafNode(tree, node, index);
				recursiveRemove(tree, node->children[index], getElementAddress(tree, node, index));
			}

		} else {
			operationResult = recursiveRemove(tree, node->children[index], elementToRemove);
		}

		if (node->children[index] != NULL) {
			if (node->children[index]->elementsCount < (tree->order - 1) / 2) {
				restoreBTreeInvariants(tree, node, index);
			}
		}

	} else {
		operationResult = B_TREE_NOTHING_FOUND;
	}

	return operationResult;
}


enum OperationResult bTreeRemove(struct BTree* tree, void* elementToRemove) {
	enum OperationResult operationResult = recursiveRemove(tree, tree->root, elementToRemove);
	/* Is the root empty? */
	if (tree->root != NULL && tree->root->elementsCount == 0) {
		struct BTreeNode* oldRoot = tree->root;
		tree->root = oldRoot->children[0];
		tree->memoryAllocatorRelease(tree->memoryAllocatorContext, oldRoot);
		tree->nodeCount--;
	}
	if (operationResult == B_TREE_SUCCESS) {
		tree->elementCount--;
	}

	return operationResult;
}

void bTreeRecursiveClear(struct BTree* tree, struct BTreeNode* node) {
	for (int i = 0; i <= node->elementsCount; i++) {
		if (node->children[i] != NULL) {
			bTreeRecursiveClear(tree, node->children[i]);
		}
	}

	tree->memoryAllocatorRelease(tree->memoryAllocatorContext, node);
}

void bTreeForEach(struct BTree* tree, bool (*processElement)(void*, void*, void*), void* argument) {
	if (tree->root != NULL) {
		recursiveForEach(tree, tree->root, processElement, argument);
	}
}

void bTreeForEachNode(struct BTree* tree, bool (*processElement)(void*, void*, void*), bool (*beforeNode)(void*, uint32_t), bool (*afterNode)(void*), void* argument) {
	if (tree->root != NULL) {
		recursiveForEachNode(tree, tree->root, 0, processElement, beforeNode, afterNode, argument);
	}
}

void bTreeClear(struct BTree* tree) {
	if (tree->root != NULL) {
		bTreeRecursiveClear(tree, tree->root);
		tree->elementCount = 0;
		tree->nodeCount = 0;
		tree->root = NULL;
	}
}

int bTreeWorstCaseNodeCountToStore(size_t treeNodeSize, size_t elementSize, int desiredSize) {
	int answer = 0;
	int order = calculateOrder(treeNodeSize, elementSize);
	int minimumElementsPerNode = mathUtilsCeilOfUint32Division(order, 2) - 1;
	assert(minimumElementsPerNode > 0);

	int leafNodesCount = mathUtilsCeilOfUint32Division(desiredSize, minimumElementsPerNode);
	for (int levelNodes = leafNodesCount; true; levelNodes = mathUtilsCeilOfUint32Division(levelNodes, minimumElementsPerNode)) {
		answer += levelNodes;
		if (levelNodes == 1) {
			break;
		}
	}

	return answer;
}
