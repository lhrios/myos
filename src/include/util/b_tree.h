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

#ifndef B_TREE_H
	#define B_TREE_H

	#include <stdbool.h>
	#include <stdint.h>
	#include <stdlib.h>

	enum OperationResult {
		B_TREE_SUCCESS,
		B_TREE_NOTHING_FOUND,
		B_TREE_OVERFLOW,
		B_TREE_KEY_ALREADY_EXISTS,
		B_TREE_NOT_ENOUGH_MEMORY
	};

	struct BTreeNode;

	struct BTree {
		int order;
		size_t elementSize;
		size_t treeNodeSize;
		void* memoryAllocatorContext;
		void* (*memoryAllocatorAcquire)(void*, size_t);
		void (*memoryAllocatorRelease)(void*, void*);
		int (*elementKeyComparator)(const void*, const void*);
		struct BTreeNode* root;
		int elementCount;
		int nodeCount;
	};

	void bTreeInitialize(struct BTree* tree, size_t treeNodeSize, size_t elementSize,
			void* memoryAllocatorContext, void* (*memoryAllocatorAcquire)(void*, size_t), void (*memoryAllocatorRelease)(void*, void*),
			int (*elementKeyComparator)(const void*, const void*));
	enum OperationResult bTreeSearch(struct BTree* tree, void* elementToSearch);
	enum OperationResult bTreeInsert(struct BTree* tree, void* elementToInsert);
	void bTreeForEach(struct BTree* btree, bool (*processElement)(void*, void*, void*), void* argument);
	void bTreeForEachNode(struct BTree* tree, bool (*processElement)(void*, void*, void*), bool (*beforeNode)(void*, uint32_t), bool (*afterNode)(void*), void* argument);
	inline __attribute__((always_inline)) int bTreeSize(struct BTree* btree) {
		return btree->elementCount;
	}
	enum OperationResult bTreeRemove(struct BTree* tree, void* elementToRemove);
	void bTreeClear(struct BTree* tree);
	int bTreeWorstCaseNodeCountToStore(size_t treeNodeSize, size_t elementSize, int desiredSize);

#endif
