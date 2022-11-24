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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "kernel/api_status_code.h"
#include "kernel/error_handler.h"
#include "kernel/limits.h"
#include "kernel/memory_manager.h"

#include "kernel/file_system/ext2_file_system.h"

#include "kernel/io/block_cache_manager.h"
#include "kernel/io/open_file_description.h"
#include "kernel/io/virtual_file_system_manager.h"

#include "util/string_stream_writer.h"
#include "util/path_utils.h"
#include "util/math_utils.h"

struct Context {
	struct Ext2FileSystem* fileSystem;
	struct Process* process;
	time_t unixTime;
	bool hasUnixTime;
};

static void initializeContext(struct Context* context, struct Ext2FileSystem* fileSystem, struct Process* process) {
	context->fileSystem = fileSystem;
	context->process = process;
	context->hasUnixTime = false;
}

static time_t getUnixTime(struct Context* context) {
	if (!context->hasUnixTime) {
		context->unixTime = cmosGetUnixTime();
		context->hasUnixTime = true;
	}

	return context->unixTime;
}

static void contextAwareAfterNodeReservationRelease(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode,
		struct OpenFileDescription* openFileDescription);
static void afterNodeReservationRelease(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* currentProcess,
		struct OpenFileDescription* openFileDescription);

static APIStatusCode commonReserveBlockById(struct Ext2FileSystem* fileSystem, uint32_t blockId, bool read, void** data) {
	struct BlockDevice* blockDevice = fileSystem->blockDevice;

	assert(fileSystem->blockSize >= blockDevice->blockSize);
	uint32_t deviceBlocksPerFileSystemBlock = fileSystem->blockSize / blockDevice->blockSize;
	uint64_t adjustedBlockId = (((uint64_t) blockId) << mathUtilsLog2ForPowerOf2(deviceBlocksPerFileSystemBlock));

	APIStatusCode result;
	if (read) {
		result = blockCacheManagerReadAndReserve(blockDevice, adjustedBlockId, deviceBlocksPerFileSystemBlock, data);
	} else {
		result = blockCacheManagerReserve(blockDevice, adjustedBlockId, deviceBlocksPerFileSystemBlock, data);
	}
	assert(result != EIO); /* All I/O errors are considered fatal. Therefore, this code will never be executed. */
	return result;
}

static APIStatusCode readAndReserveBlockById(struct Ext2FileSystem* fileSystem, uint32_t blockId, void** data) {
	return commonReserveBlockById(fileSystem, blockId, true, data);
}

static APIStatusCode reserveBlockById(struct Ext2FileSystem* fileSystem, uint32_t blockId, void** data) {
	return commonReserveBlockById(fileSystem, blockId, false, data);
}

static void releaseCachedBlockReservation(struct Ext2FileSystem* fileSystem, uint32_t blockId, bool modified) {
	struct BlockDevice* blockDevice = fileSystem->blockDevice;

	uint32_t blocksPerBlock = fileSystem->blockSize / blockDevice->blockSize;
	uint64_t adjustedBlockId = (((uint64_t) blockId) << mathUtilsLog2ForPowerOf2(blocksPerBlock));

	blockCacheManageReleaseReservation(blockDevice, adjustedBlockId, modified);
}

static bool canIncreaseSize(size_t currentSize, size_t increment) {
	return increment <= FILE_MAX_SIZE && FILE_MAX_SIZE - increment >= currentSize;
}

static size_t localGetSize(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode) {
	if (fileSystem->superBlock.s_rev_level >= EXT2_DYNAMIC_REV) {
		if (S_ISREG(iNode->i_mode)) {
			assert(iNode->i_dir_acl == 0);
		}
	}
	return iNode->i_size;
}

static off_t getSize(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	size_t size = localGetSize(fileSystem, iNode);
	assert(size <= FILE_MAX_SIZE);
	return (off_t) size;
}

static struct Ext2BlockGroupDescriptor* getBlockGroupDescriptor(struct Ext2FileSystem* fileSystem, uint32_t blockGroupIndex) {
	assert(blockGroupIndex < fileSystem->blockGroupsCount);

	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&fileSystem->blockGroupDescriptorsPageFrameList);
	uint32_t doubleLinkedListElementIndex = blockGroupIndex / fileSystem->blockGroupDescriptorsPerBlock;
	uint32_t i = 0;
	while (doubleLinkedListElement != NULL) {
		if (i == doubleLinkedListElementIndex) {
			return &((struct Ext2BlockGroupDescriptor*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement))[blockGroupIndex % fileSystem->blockGroupDescriptorsPerBlock];
		}

		doubleLinkedListElement = doubleLinkedListElement->next;
		i++;
	}

	assert(false);
	return NULL;
}

static APIStatusCode readINode(struct Ext2FileSystem* fileSystem, uint32_t iNodeIndex, struct Ext2INode* inode) {
	assert(iNodeIndex > 0);

	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;
	uint32_t blockGroupIndex = (iNodeIndex - 1) / superBlock->s_inodes_per_group;
	uint32_t localINodeIndex = (iNodeIndex - 1) % superBlock->s_inodes_per_group;

	struct Ext2BlockGroupDescriptor* blockGroupDescriptor = getBlockGroupDescriptor(fileSystem, blockGroupIndex);

	void* data;
	uint32_t blockId = blockGroupDescriptor->bg_inode_table + localINodeIndex / fileSystem->iNodesPerBlock;
	APIStatusCode result = readAndReserveBlockById(fileSystem, blockId, &data);
	if (result == SUCCESS) {
		memcpy(inode, data + (localINodeIndex % fileSystem->iNodesPerBlock) * superBlock->s_inode_size, sizeof(struct Ext2INode));
		releaseCachedBlockReservation(fileSystem, blockId, false);
	}
	return result;
}

static APIStatusCode writeINode(struct Ext2FileSystem* fileSystem, uint32_t iNodeIndex, struct Ext2INode* inode) {
	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;
	uint32_t blockGroupIndex = (iNodeIndex - 1) / superBlock->s_inodes_per_group;
	uint32_t localINodeIndex = (iNodeIndex - 1) % superBlock->s_inodes_per_group;

	struct Ext2BlockGroupDescriptor* blockGroupDescriptor = getBlockGroupDescriptor(fileSystem, blockGroupIndex);

	void* data;
	uint32_t blockId = blockGroupDescriptor->bg_inode_table + localINodeIndex / fileSystem->iNodesPerBlock;
	APIStatusCode result = readAndReserveBlockById(fileSystem, blockId, &data);
	if (result == SUCCESS) {
		memcpy(data + (localINodeIndex % fileSystem->iNodesPerBlock) * superBlock->s_inode_size, inode, sizeof(struct Ext2INode));
		releaseCachedBlockReservation(fileSystem, blockId, true);
	}

	return result;
}

static inline __attribute__((always_inline)) uint32_t getINodeBlockCount(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode) {
	return iNode->i_blocks / ((1024 << fileSystem->superBlock.s_log_block_size) / 512);
}

static APIStatusCode getSingleIndirectionDataBlockId(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t localDataBlockIndex, void* singleIndirectionDataBlockArray, uint32_t* dataBlockId) {
	assert(localDataBlockIndex < fileSystem->dataBlockIndexesPerBlock);
	*dataBlockId = ((uint32_t*) singleIndirectionDataBlockArray)[localDataBlockIndex];
	return SUCCESS;
}

static APIStatusCode getDoubleIndirectionDataBlockId(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t localDataBlockIndex, void* doubleIndirectionDataBlockArray, uint32_t* dataBlockId) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	uint32_t index = localDataBlockIndex / dataBlockIndexesPerBlock;

	assert(localDataBlockIndex < dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	void* singleIndirectionDataBlockArray;
	uint32_t blockId = ((uint32_t*) doubleIndirectionDataBlockArray)[index];
	APIStatusCode result = readAndReserveBlockById(fileSystem, blockId, &singleIndirectionDataBlockArray);
	if (result == SUCCESS) {
		result = getSingleIndirectionDataBlockId(fileSystem, iNode, localDataBlockIndex % dataBlockIndexesPerBlock, singleIndirectionDataBlockArray, dataBlockId);
		releaseCachedBlockReservation(fileSystem, blockId, false);
	}
	return result;
}

static APIStatusCode getTripleIndirectionDataBlockId(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t localDataBlockIndex, void* tripleIndirectionDataBlockArray, uint32_t* dataBlockId) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	uint32_t index = localDataBlockIndex / (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	assert(localDataBlockIndex < dataBlockIndexesPerBlock * dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	void* doubleIndirectionDataBlockArray;
	uint32_t blockId = ((uint32_t*) tripleIndirectionDataBlockArray)[index];
	APIStatusCode result = readAndReserveBlockById(fileSystem, blockId, &doubleIndirectionDataBlockArray);
	if (result == SUCCESS) {
		result = getDoubleIndirectionDataBlockId(fileSystem, iNode, localDataBlockIndex % (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock), doubleIndirectionDataBlockArray, dataBlockId);
		releaseCachedBlockReservation(fileSystem, blockId, false);
	}
	return result;
}

static APIStatusCode getInodeDataBlockId(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t dataBlockIndex, void** data, uint32_t* dataBlockId) {
	APIStatusCode result = SUCCESS;
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;

	if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT) {
		*dataBlockId = iNode->i_block[dataBlockIndex];

	} else if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT + dataBlockIndexesPerBlock) {
		/* Single indirection blocks. */
		void* singleIndirectionDataBlockArray;
		uint32_t blockId = iNode->i_block[EXT2_SINGLE_INDIRECTION_ENTRY_INDEX];
		result = readAndReserveBlockById(fileSystem, blockId, &singleIndirectionDataBlockArray);
		if (result == SUCCESS) {
			result = getSingleIndirectionDataBlockId(fileSystem, iNode, dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT, singleIndirectionDataBlockArray, dataBlockId);
			releaseCachedBlockReservation(fileSystem, blockId, false);
		}

	} else if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT + dataBlockIndexesPerBlock + dataBlockIndexesPerBlock * dataBlockIndexesPerBlock) {
		/* Double indirection blocks. */
		void* doubleIndirectionDataBlockArray;
		uint32_t blockId = iNode->i_block[EXT2_DOUBLE_INDIRECTION_ENTRY_INDEX];
		result = readAndReserveBlockById(fileSystem, blockId, &doubleIndirectionDataBlockArray);
		if (result == SUCCESS) {
			result = getDoubleIndirectionDataBlockId(fileSystem, iNode, dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT - dataBlockIndexesPerBlock, doubleIndirectionDataBlockArray, dataBlockId);
			releaseCachedBlockReservation(fileSystem, blockId, false);
		}

	} else {
		/* Triple indirection blocks. */
		void* tripleIndirectionDataBlockArray;
		uint32_t blockId = iNode->i_block[EXT2_TRIPLE_INDIRECTION_ENTRY_INDEX];
		result = readAndReserveBlockById(fileSystem, blockId, &tripleIndirectionDataBlockArray);
		if (result == SUCCESS) {
			uint32_t localDataBlockIndex = (dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT - dataBlockIndexesPerBlock - dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
			result = getTripleIndirectionDataBlockId(fileSystem, iNode, localDataBlockIndex, tripleIndirectionDataBlockArray, dataBlockId);
			releaseCachedBlockReservation(fileSystem, blockId, false);
		}
	}

	return result;
}

static APIStatusCode readInodeDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t dataBlockIndex, void** data, uint32_t* dataBlockId) {
	APIStatusCode result = SUCCESS;
	result = getInodeDataBlockId(fileSystem, iNode, dataBlockIndex, data, dataBlockId);
	if (result == SUCCESS) {
		assert(*dataBlockId != 0);
		result = readAndReserveBlockById(fileSystem, *dataBlockId, data);
	}
	return result;
}

static APIStatusCode readNextLinkedDirectoryEntry(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode,
		struct Ext2LinkedDirectoryEntry** linkedDirectoryEntry, int* offset, uint32_t* dataBlockId, bool* endOfDirectory) {
	assert(S_ISDIR(iNode->i_mode));
	APIStatusCode result = SUCCESS;

	*endOfDirectory = true;
	while (*offset < localGetSize(fileSystem, iNode)) {
		uint32_t dataBlockIndex = *offset / fileSystem->blockSize;
		void* data;
		result = readInodeDataBlock(fileSystem, iNode, dataBlockIndex, &data, dataBlockId);
		if (result == SUCCESS) {
			*linkedDirectoryEntry = data + (*offset) % fileSystem->blockSize;

			assert((*linkedDirectoryEntry)->rec_len != 0);
			*offset = *offset + (*linkedDirectoryEntry)->rec_len;

			if ((*linkedDirectoryEntry)->inode != 0) {
				*endOfDirectory = false;
				break;

			} else {
				releaseCachedBlockReservation(fileSystem, *dataBlockId, false);
			}

		} else {
			break;
		}
	}

	return result;
}

static mode_t getMode(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = (struct Ext2VirtualFileSystemNode*) virtualFileSystemNode;
	return ext2VirtualFileSystemNode->iNode.i_mode;
}

static uint32_t calculateDataBlockCountFromSize(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;

	uint32_t resultWithoutMetaData = mathUtilsCeilOfUint32Division(localGetSize(fileSystem, iNode), fileSystem->blockSize);
	uint32_t result = resultWithoutMetaData;

	if (resultWithoutMetaData > EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT
					+ dataBlockIndexesPerBlock
					+ dataBlockIndexesPerBlock * dataBlockIndexesPerBlock) {
		uint32_t tripleIndirectionDataBlockCount = resultWithoutMetaData - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT
				- dataBlockIndexesPerBlock
				- dataBlockIndexesPerBlock * dataBlockIndexesPerBlock;
		assert(0 < tripleIndirectionDataBlockCount && tripleIndirectionDataBlockCount <= dataBlockIndexesPerBlock * dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		resultWithoutMetaData -= tripleIndirectionDataBlockCount;
		result += mathUtilsCeilOfUint32Division(tripleIndirectionDataBlockCount, dataBlockIndexesPerBlock * dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		result += mathUtilsCeilOfUint32Division(tripleIndirectionDataBlockCount, dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		result += mathUtilsCeilOfUint32Division(tripleIndirectionDataBlockCount, dataBlockIndexesPerBlock);
	}
	if (resultWithoutMetaData > EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT
					+ dataBlockIndexesPerBlock) {
		uint32_t doubleIndirectionDataBlockCount = resultWithoutMetaData - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT
				- dataBlockIndexesPerBlock;
		assert(0 < doubleIndirectionDataBlockCount && doubleIndirectionDataBlockCount <= dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		resultWithoutMetaData -= doubleIndirectionDataBlockCount;
		result += mathUtilsCeilOfUint32Division(doubleIndirectionDataBlockCount, dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		result += mathUtilsCeilOfUint32Division(doubleIndirectionDataBlockCount, dataBlockIndexesPerBlock);
	}
	if (resultWithoutMetaData > EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT) {
		uint32_t singleIndirectionDataBlockCount = resultWithoutMetaData - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT;
		assert(0 < singleIndirectionDataBlockCount && singleIndirectionDataBlockCount <= dataBlockIndexesPerBlock);
		resultWithoutMetaData -= singleIndirectionDataBlockCount;
		result += mathUtilsCeilOfUint32Division(singleIndirectionDataBlockCount, dataBlockIndexesPerBlock);
	}

	return result;
}

static APIStatusCode open(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process, struct OpenFileDescription** openFileDescription, int flags) {
	struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	APIStatusCode result = SUCCESS;
	if (fileSystem->superBlock.s_rev_level >= EXT2_DYNAMIC_REV) {
		if (S_ISREG(iNode->i_mode)) {
			if (iNode->i_dir_acl != 0) {
				result = EFBIG; /* We do not support large files. */
			} else if (iNode->i_size > FILE_MAX_SIZE) {
				result = EFBIG;
			}
		}
	}
	if (result == SUCCESS) {
		if (((!S_ISLNK(iNode->i_mode) || localGetSize(fileSystem, iNode) > EXT2_SYMBOLIC_LINK_SELF_CONTAINED_DATA_MAX_SIZE))
				&& iNode->i_blocks != calculateDataBlockCountFromSize(fileSystem, iNode) * (fileSystem->blockSize / 512)) {
			result = EPERM; /* We do not support files with holes (https://www.halolinux.us/kernel-reference/file-holes.html). */
		}
	}

	return result;
}

static APIStatusCode contextAwareReadDirectoryEntry(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode,
		struct OpenFileDescription* openFileDescription, struct dirent* direntInstance, bool* endOfDirectory) {
	struct Ext2FileSystem* fileSystem = context->fileSystem;
	struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;

	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	assert(S_ISDIR(iNode->i_mode));

	uint32_t dataBlockId;
	APIStatusCode result = readNextLinkedDirectoryEntry(fileSystem, iNode, &linkedDirectoryEntry, &openFileDescription->offset, &dataBlockId, endOfDirectory);
	if (!*endOfDirectory) {
		direntInstance->d_ino = linkedDirectoryEntry->inode;
		memcpy(direntInstance->d_name, &linkedDirectoryEntry->nameFirstCharacter, linkedDirectoryEntry->name_len);
		releaseCachedBlockReservation(fileSystem, dataBlockId, false);
		assert(linkedDirectoryEntry->name_len <= FILE_NAME_MAX_LENGTH - 1);
		direntInstance->d_name[linkedDirectoryEntry->name_len] = '\0';
	}

	iNode->i_atime = getUnixTime(context);
	ext2VirtualFileSystemNode->isDirty = true;

	return result;
}

static APIStatusCode readDirectoryEntry(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process,
		struct OpenFileDescription* openFileDescription, struct dirent* direntInstance, bool* endOfDirectory) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareReadDirectoryEntry(&context, ext2VirtualFileSystemNode, openFileDescription, direntInstance, endOfDirectory);
}

static APIStatusCode getExt2VirtualFileSystemNodeByINodeIndex(struct Ext2FileSystem* fileSystem, uint32_t iNodeIndex, struct Ext2VirtualFileSystemNode** ext2VirtualFileSystemNode) {
	APIStatusCode result = SUCCESS;

	struct Ext2VirtualFileSystemNode node;
	node.iNodeIndex = iNodeIndex;

	struct Ext2VirtualFileSystemNode* selectedNode = &node;

	if (B_TREE_SUCCESS != bTreeSearch(&fileSystem->ext2VirtualFileSystemNodeByINodeIndex, &selectedNode)) {
		if (doubleLinkedListSize(&fileSystem->availableExt2VirtualFileSystemNodesList) > 0) {
			struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&fileSystem->availableExt2VirtualFileSystemNodesList);

			uint32_t address = ((uint32_t) doubleLinkedListElement) - offsetof(struct Ext2VirtualFileSystemNode, availableListElement);
			selectedNode = (struct Ext2VirtualFileSystemNode*) address;
			assert(&selectedNode->availableListElement == doubleLinkedListElement);
			assert(selectedNode->virtualFileSystemNode.usageCount == 0);
			memset(&selectedNode->virtualFileSystemNode, 0, sizeof(struct VirtualFileSystemNode));
			selectedNode->virtualFileSystemNode.operations = &fileSystem->operations;
			selectedNode->iNodeIndex = iNodeIndex;
			selectedNode->isDirty = false;

			result = readINode(fileSystem, iNodeIndex, &selectedNode->iNode);
			if (result == SUCCESS) {
				enum OperationResult operationResult = bTreeInsert(&fileSystem->ext2VirtualFileSystemNodeByINodeIndex, &selectedNode);
				if (B_TREE_SUCCESS != operationResult) {
					assert(operationResult == B_TREE_NOT_ENOUGH_MEMORY);
					result = ENOMEM;
					doubleLinkedListInsertBeforeFirst(&fileSystem->availableExt2VirtualFileSystemNodesList, &selectedNode->availableListElement);
					selectedNode = NULL;
				}
			}

		} else {
			selectedNode = NULL;
			result = ENOMEM;
		}
	}

	*ext2VirtualFileSystemNode = selectedNode;
	return result;
}

static APIStatusCode mergeWithSymbolicLinkPath(struct VirtualFileSystemNode* virtualFileSystemNode, struct PathUtilsContext* pathUtilsContext, int lastPathSegmentIndex) {
	struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = (void*) virtualFileSystemNode;
	assert(S_ISLNK(ext2VirtualFileSystemNode->iNode.i_mode));

	struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	APIStatusCode result = SUCCESS;

	bool releaseCachedBlock = false;
	uint32_t dataBlockId;
	uint32_t symbolicLinkPathLength = localGetSize(fileSystem, iNode);
	const char* symbolicLinkPath = NULL;
	if (symbolicLinkPathLength <= EXT2_SYMBOLIC_LINK_SELF_CONTAINED_DATA_MAX_SIZE) {
		symbolicLinkPath = (char*) iNode->i_block;

	} else if (symbolicLinkPathLength < PATH_MAX_LENGTH) {
		void *data;
		result = readInodeDataBlock(fileSystem, iNode, 0, &data, &dataBlockId);
		if (result == SUCCESS) {
			symbolicLinkPath = (char*) data;
			releaseCachedBlock = true;
		}

	} else {
		result = ENAMETOOLONG;
	}

	if (result == SUCCESS) {
		struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
		if (doubleLinkedListElement != NULL) {
			struct StringStreamWriter stringStreamWriter;
			assert(PATH_MAX_LENGTH <= PAGE_FRAME_SIZE);
			char* buffer = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
			stringStreamWriterInitialize(&stringStreamWriter, buffer, PATH_MAX_LENGTH);

			if (pathUtilsIsAbsolute(symbolicLinkPath)) {
				/* Case #1: "symbolic link path" + "original path that has not been processed yet". */
				streamWriterWriteString(&stringStreamWriter.streamWriter, symbolicLinkPath, symbolicLinkPathLength);

				for (int i = lastPathSegmentIndex + 1; i < pathUtilsContext->segmentCount; i++) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '/');
					streamWriterWriteString(&stringStreamWriter.streamWriter, pathUtilsContext->segments[i], UINT_MAX);
				}

			} else {
				/* Case #2: "original path already processed" + "symbolic link path" + "original path that has not been processed yet". */
				for (int i = 0; i < lastPathSegmentIndex; i++) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '/');
					streamWriterWriteString(&stringStreamWriter.streamWriter, pathUtilsContext->segments[i], UINT_MAX);
				}
				streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '/');
				streamWriterWriteString(&stringStreamWriter.streamWriter, symbolicLinkPath, symbolicLinkPathLength);
				for (int i = lastPathSegmentIndex + 1; i < pathUtilsContext->segmentCount; i++) {
					streamWriterWriteCharacter(&stringStreamWriter.streamWriter, '/');
					streamWriterWriteString(&stringStreamWriter.streamWriter, pathUtilsContext->segments[i], UINT_MAX);
				}
			}

			if (stringStreamWriterGetAvailable(&stringStreamWriter) > 0) {
				stringStreamWriterForceTerminationCharacter(&stringStreamWriter);
				strcpy(pathUtilsContext->buffer, buffer);

			} else {
				result = ENAMETOOLONG;
			}

			memoryManagerReleasePageFrame(doubleLinkedListElement, -1);

		} else {
			result = ENOMEM;
		}
	}

	if (releaseCachedBlock) {
		releaseCachedBlockReservation(fileSystem, dataBlockId, false);
	}

	return result;
}

static APIStatusCode contextAwareRead(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct OpenFileDescription* openFileDescription,
		void* buffer, size_t bufferSize, size_t* count) {
	APIStatusCode result = SUCCESS;

	struct Ext2FileSystem* fileSystem = context->fileSystem;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	if (S_ISREG(iNode->i_mode) || S_ISLNK(iNode->i_mode)) {
		uint32_t size = localGetSize(fileSystem, iNode);
		*count = 0;

		while (result == SUCCESS && openFileDescription->offset < size && bufferSize > 0) {
			int intraBlockOffset = openFileDescription->offset % fileSystem->blockSize;
			uint32_t dataBlockIndex = openFileDescription->offset / fileSystem->blockSize;

			int localCount = size - openFileDescription->offset;
			localCount = mathUtilsMin(localCount, bufferSize);
			localCount = mathUtilsMin(localCount, fileSystem->blockSize);
			if (localCount + intraBlockOffset > fileSystem->blockSize) {
				localCount -= localCount + intraBlockOffset - fileSystem->blockSize;
			}

			bool releaseCachedBlock = false;
			void* data;
			uint32_t dataBlockId = 0;
			if (S_ISREG(iNode->i_mode) || (S_ISLNK(iNode->i_mode) && size > EXT2_SYMBOLIC_LINK_SELF_CONTAINED_DATA_MAX_SIZE)) {
				result = readInodeDataBlock(fileSystem, iNode, dataBlockIndex, &data, &dataBlockId);
				if (result == SUCCESS) {
					releaseCachedBlock = true;
				}

			} else {
				data = iNode->i_block;
			}

			if (result == SUCCESS) {
				memcpy(buffer, data + intraBlockOffset, localCount);
				if (releaseCachedBlock) {
					releaseCachedBlockReservation(fileSystem, dataBlockId, false);
				}
				openFileDescription->offset += localCount;
				*count += localCount;
				bufferSize -= localCount;
				buffer += localCount;
			}
		}

		iNode->i_atime = getUnixTime(context);
		ext2VirtualFileSystemNode->isDirty = true;

	} else {
		result = EPERM;
	}

	return result;
}

static APIStatusCode read(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription,
		void* buffer, size_t bufferSize, size_t* count) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareRead(&context, ext2VirtualFileSystemNode, openFileDescription, buffer, bufferSize, count);
}

static enum OpenFileDescriptionOffsetRepositionPolicy getOpenFileDescriptionOffsetRepositionPolicy(struct VirtualFileSystemNode* virtualFileSystemNode) {
	struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	if (S_ISREG(iNode->i_mode)) {
		return REPOSITION_FREELY;
	} else if (S_ISDIR(iNode->i_mode)) {
		return ALWAYS_REPOSITION_TO_ZERO;
	} else {
		return REPOSITION_NOT_ALLOWED;
	}
}

static APIStatusCode status(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription, struct stat* statInstance) {
	struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	statInstance->st_atime = iNode->i_atime;
	statInstance->st_blksize = fileSystem->blockSize;
	statInstance->st_blocks = getINodeBlockCount(fileSystem, iNode);
	statInstance->st_ctime = iNode->i_ctime;
	statInstance->st_dev = fileSystem->blockDevice->id;
	statInstance->st_gid = iNode->i_gid;
	statInstance->st_ino = ext2VirtualFileSystemNode->iNodeIndex;
	statInstance->st_mtime = iNode->i_mtime;
	statInstance->st_nlink = iNode->i_links_count;
	statInstance->st_size = iNode->i_size;
	statInstance->st_uid = iNode->i_uid;

	statInstance->st_atim.tv_sec = statInstance->st_atime;
	statInstance->st_mtim.tv_sec = statInstance->st_mtime;
	statInstance->st_ctim.tv_sec = statInstance->st_ctime;

	return SUCCESS;
}

static void* memoryAllocatorAcquire(int reservationId, size_t size) {
	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, reservationId);
	if (doubleLinkedListElement != NULL) {
		return (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);

	} else {
		return NULL;
	}
}

static void memoryAllocatorRelease(int reservationId, void* pointer) {
	struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerGetPageFrameDoubleLinkedListElement((uint32_t) pointer);
	memoryManagerReleasePageFrame(doubleLinkedListElement, reservationId);
}

static int compare(struct Ext2VirtualFileSystemNode** element1, struct Ext2VirtualFileSystemNode** element2) {
	uint32_t iNodeIndex1 = (*element1)->iNodeIndex;
	uint32_t iNodeIndex2 = (*element2)->iNodeIndex;

	if (iNodeIndex1 == iNodeIndex2) {
		return 0;
	} else if (iNodeIndex1 > iNodeIndex2) {
		return 1;
	} else {
		return -1;
	}
}

static bool isValidExt2SuperBlock(struct Ext2SuperBlock* superBlock) {
	bool isValid = true;

	uint32_t blockSize = 1024 << superBlock->s_log_block_size;

	isValid = isValid && superBlock->s_magic == EXT2_SUPER_MAGIC;
	isValid = isValid && ((blockSize == 1024 && superBlock->s_first_data_block == 1) || (blockSize > 1024 && superBlock->s_first_data_block == 0));
	isValid = isValid && superBlock->s_blocks_count >= superBlock->s_free_blocks_count;
	isValid = isValid && superBlock->s_blocks_count >= superBlock->s_r_blocks_count;
	isValid = isValid && superBlock->s_inodes_count >= superBlock->s_free_inodes_count;
	isValid = isValid && superBlock->s_inode_size >= sizeof(struct Ext2INode);
	isValid = isValid && superBlock->s_blocks_per_group <= 8 * blockSize;
	isValid = isValid && superBlock->s_inodes_per_group <= 8 * blockSize;

	return isValid;
}

static bool isSupportedExt2FileSystem(struct Ext2SuperBlock* superBlock) {
	bool isSupported = true;

	size_t blockSize = 1024 << superBlock->s_log_block_size;
	size_t fragmentSize;
	if(superBlock->s_log_frag_size > 0) {
		fragmentSize = 1024 << superBlock->s_log_frag_size;
	} else {
		fragmentSize = 1024 >> -superBlock->s_log_frag_size;
	}

	isSupported = isSupported && fragmentSize == blockSize;
	isSupported = isSupported && blockSize <= PAGE_FRAME_SIZE;
	isSupported = isSupported && superBlock->s_rev_level == EXT2_DYNAMIC_REV;
	isSupported = isSupported && superBlock->s_feature_incompat == EXT2_FEATURE_INCOMPAT_FILETYPE;

	isSupported = isSupported && (superBlock->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) != 0;
	isSupported = isSupported && (superBlock->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE) != 0;
	isSupported = isSupported && (superBlock->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_BTREE_DIR) == 0;

	return isSupported;
}

static size_t calculateMinimumLinkedDirectoryEntrySize(size_t nameLength) {
	size_t size = sizeof(struct Ext2LinkedDirectoryEntry) - sizeof(char) + nameLength * sizeof(char);
	while (size % 4 != 0) {
		size++;
	}
	return size;
}

static APIStatusCode acquireINode(struct Ext2FileSystem* fileSystem, uint32_t* iNodeIndex, bool willBecomeDirectory) {
	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;

	APIStatusCode result = SUCCESS;

	bool found = false;
	int i = 0;
	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&fileSystem->blockGroupDescriptorsPageFrameList);
	while (!found && result == SUCCESS && doubleLinkedListElement != NULL) {
		struct Ext2BlockGroupDescriptor* blockGroupDescriptors = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		doubleLinkedListElement = doubleLinkedListElement->next;

		for (int j = 0; !found && result == SUCCESS && j < fileSystem->blockGroupDescriptorsPerBlock && i < fileSystem->blockGroupsCount; j++) {
			struct Ext2BlockGroupDescriptor* blockGroupDescriptor = &blockGroupDescriptors[j];
			if (blockGroupDescriptor->bg_free_inodes_count > 0) {
				uint8_t* iNodeBitmap;
				result = readAndReserveBlockById(fileSystem, blockGroupDescriptor->bg_inode_bitmap, (void**) &iNodeBitmap);
				if (result == SUCCESS) {
					for (uint32_t k = 0; k < superBlock->s_inodes_count; k++) {
						uint8_t value = iNodeBitmap[k / 8];
						bool used = (value & (1 << (k % 8))) != 0;
						if (!used) {
							found = true;
							if (willBecomeDirectory) {
								blockGroupDescriptor->bg_used_dirs_count++;
							}
							blockGroupDescriptor->bg_free_inodes_count--;
							superBlock->s_free_inodes_count--;
							value = value | (1 << (k % 8));
							iNodeBitmap[k / 8] = value;
							releaseCachedBlockReservation(fileSystem, blockGroupDescriptor->bg_inode_bitmap, true);
							fileSystem->isSuperBlockOrBlockGroupsDirty = true;
							*iNodeIndex = superBlock->s_inodes_per_group * i + k + 1;
							break;
						}
					}

				} else {
					break;
				}
			}

			i++;
		}
	}

	if (!found && result == SUCCESS) {
		result = ENOSPC;
	}

	return result;
}

/* If the following method always fails, it will be a fatal error. */
static void releaseINode(struct Ext2FileSystem* fileSystem, uint32_t iNodeIndex, bool wasDirectory) {
	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;
	assert(iNodeIndex >= superBlock->s_first_ino);

	uint32_t blockGroupIndex = (iNodeIndex - 1) / superBlock->s_inodes_per_group;
	uint32_t localINodeIndex = (iNodeIndex - 1) % superBlock->s_inodes_per_group;
	struct Ext2BlockGroupDescriptor* blockGroupDescriptor = getBlockGroupDescriptor(fileSystem, blockGroupIndex);
	uint8_t* iNodeBitmap;
	APIStatusCode result = readAndReserveBlockById(fileSystem, blockGroupDescriptor->bg_inode_bitmap, (void**) &iNodeBitmap);
	if (result != SUCCESS) {
		errorHandlerFatalError("There was a fatal error while trying to read file system's metadata: %s", sys_errlist[result]);
	}
	uint8_t value = iNodeBitmap[localINodeIndex / 8];
	assert((value & (1 << (localINodeIndex % 8))) != 0);
	value = value & ~(1 << (localINodeIndex % 8));
	iNodeBitmap[localINodeIndex / 8] = value;
	releaseCachedBlockReservation(fileSystem, blockGroupDescriptor->bg_inode_bitmap, true);
	superBlock->s_free_inodes_count++;
	blockGroupDescriptor->bg_free_inodes_count++;
	if (wasDirectory) {
		blockGroupDescriptor->bg_used_dirs_count--;
	}
	fileSystem->isSuperBlockOrBlockGroupsDirty = true;
}

static APIStatusCode acquireDataBlock(struct Ext2FileSystem* fileSystem, uint32_t* dataBlockId) {
	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;

	APIStatusCode result = SUCCESS;

	bool found = false;
	int i = 0;
	struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListFirst(&fileSystem->blockGroupDescriptorsPageFrameList);
	while (!found && result == SUCCESS && doubleLinkedListElement != NULL) {
		struct Ext2BlockGroupDescriptor* blockGroupDescriptors = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
		doubleLinkedListElement = doubleLinkedListElement->next;

		for (int j = 0; !found && result == SUCCESS && j < fileSystem->blockGroupDescriptorsPerBlock && i < fileSystem->blockGroupsCount; j++) {
			struct Ext2BlockGroupDescriptor* blockGroupDescriptor = &blockGroupDescriptors[j];
			if (blockGroupDescriptor->bg_free_blocks_count > 0) { // TODO: Honor the minimum amount reserved to root user
				uint8_t* blockBitmap;
				result = readAndReserveBlockById(fileSystem, blockGroupDescriptor->bg_block_bitmap, (void**) &blockBitmap);
				if (result == SUCCESS) {
					uint32_t blockCount;
					/* Is it the last one? */
					if (i + 1 == fileSystem->blockGroupsCount) {
						blockCount = superBlock->s_blocks_count % superBlock->s_blocks_per_group;
					} else {
						blockCount = superBlock->s_blocks_per_group;
					}

					for (uint32_t k = 0; k < blockCount; k++) {
						uint8_t value = blockBitmap[k / 8];
						bool used = (value & (1 << (k % 8))) != 0;
						if (!used) {
							found = true;
							blockGroupDescriptor->bg_free_blocks_count--;
							superBlock->s_free_blocks_count--;
							value = value | (1 << (k % 8));
							blockBitmap[k / 8] = value;
							releaseCachedBlockReservation(fileSystem, blockGroupDescriptor->bg_block_bitmap, true);
							fileSystem->isSuperBlockOrBlockGroupsDirty = true;
							*dataBlockId = superBlock->s_blocks_per_group * i + k + superBlock->s_first_data_block;
							break;
						}
					}

				} else {
					break;
				}
			}

			i++;
		}
	}

	if (!found && result == SUCCESS) {
		result = ENOSPC;
	}

	return result;
}

/* If the following method always fails, it will be a fatal error. */
static void releaseDataBlock(struct Ext2FileSystem* fileSystem, uint32_t dataBlockId) {
	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;
	assert(dataBlockId >= superBlock->s_first_data_block);

	uint32_t blockGroupIndex = (dataBlockId - superBlock->s_first_data_block) / superBlock->s_blocks_per_group;
	struct Ext2BlockGroupDescriptor* blockGroupDescriptor = getBlockGroupDescriptor(fileSystem, blockGroupIndex);
	uint8_t* blockBitmap;
	APIStatusCode result = readAndReserveBlockById(fileSystem, blockGroupDescriptor->bg_block_bitmap, (void**) &blockBitmap);
	if (result != SUCCESS) {
		errorHandlerFatalError("There was a fatal error while trying to read file system's metadata: %s", sys_errlist[result]);
	}
	uint32_t dataBlockIndex = (dataBlockId - superBlock->s_first_data_block) % superBlock->s_blocks_per_group;
	uint8_t value = blockBitmap[dataBlockIndex / 8];
	assert((value & (1 << (dataBlockIndex % 8))) != 0);
	value = value & ~(1 << (dataBlockIndex % 8));
	blockBitmap[dataBlockIndex / 8] = value;
	releaseCachedBlockReservation(fileSystem, blockGroupDescriptor->bg_block_bitmap, true);
	superBlock->s_free_blocks_count++;
	blockGroupDescriptor->bg_free_blocks_count++;
	fileSystem->isSuperBlockOrBlockGroupsDirty = true;
}

static APIStatusCode appendSingleIndirectionDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t* singleIndirectionDataBlockId, uint32_t localDataBlockIndex, uint32_t dataBlockId) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	assert(0 <= localDataBlockIndex && localDataBlockIndex < dataBlockIndexesPerBlock);

	APIStatusCode result = SUCCESS;

	uint32_t metaDataBlockId = 0;
	uint32_t* singleIndirectionDataBlockArray;
	if (localDataBlockIndex == 0) {
		result = acquireDataBlock(fileSystem, &metaDataBlockId);
		if (result == SUCCESS) {
			assert(metaDataBlockId != 0);
			*singleIndirectionDataBlockId = metaDataBlockId;
			iNode->i_blocks += fileSystem->blockSize / 512;

			result = reserveBlockById(fileSystem, metaDataBlockId, (void**) &singleIndirectionDataBlockArray);
			if (result == SUCCESS) {
				memset(singleIndirectionDataBlockArray, 0, fileSystem->blockSize);
			}
		}

	} else {
		assert(*singleIndirectionDataBlockId != 0);
		result = readAndReserveBlockById(fileSystem, *singleIndirectionDataBlockId, (void**) &singleIndirectionDataBlockArray);
	}
	if (result == SUCCESS) {
		singleIndirectionDataBlockArray[localDataBlockIndex] = dataBlockId;
		releaseCachedBlockReservation(fileSystem, *singleIndirectionDataBlockId, true);
		iNode->i_blocks += fileSystem->blockSize / 512;
	}
	if (result != SUCCESS && metaDataBlockId != 0) {
		iNode->i_blocks -= fileSystem->blockSize / 512;
		releaseDataBlock(fileSystem, metaDataBlockId);
	}

	return result;
}

static APIStatusCode appendDoubleIndirectionDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t* doubleIndirectionDataBlockId, uint32_t localDataBlockIndex, uint32_t dataBlockId) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	assert(0 <= localDataBlockIndex && localDataBlockIndex < dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	APIStatusCode result = SUCCESS;

	uint32_t metaDataBlockId = 0;
	uint32_t* doubleIndirectionDataBlockArray;
	if (localDataBlockIndex == 0) {
		result = acquireDataBlock(fileSystem, &metaDataBlockId);
		if (result == SUCCESS) {
			assert(metaDataBlockId != 0);
			*doubleIndirectionDataBlockId = metaDataBlockId;
			iNode->i_blocks += fileSystem->blockSize / 512;
			result = reserveBlockById(fileSystem, metaDataBlockId, (void**) &doubleIndirectionDataBlockArray);
			if (result == SUCCESS) {
				memset(doubleIndirectionDataBlockArray, 0, fileSystem->blockSize);
			}
		}

	} else {
		assert(*doubleIndirectionDataBlockId != 0);
		result = readAndReserveBlockById(fileSystem, *doubleIndirectionDataBlockId, (void**) &doubleIndirectionDataBlockArray);
	}
	if (result == SUCCESS) {
		result = appendSingleIndirectionDataBlock(fileSystem, iNode, &doubleIndirectionDataBlockArray[localDataBlockIndex / dataBlockIndexesPerBlock], localDataBlockIndex % dataBlockIndexesPerBlock, dataBlockId);
		releaseCachedBlockReservation(fileSystem, *doubleIndirectionDataBlockId, true);
	}
	if (result != SUCCESS && metaDataBlockId != 0) {
		iNode->i_blocks -= fileSystem->blockSize / 512;
		releaseDataBlock(fileSystem, metaDataBlockId);
	}

	return result;
}

static APIStatusCode appendTripleIndirectionDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t* tripleIndirectionDataBlockId, uint32_t localDataBlockIndex, uint32_t dataBlockId) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	assert(0 <= localDataBlockIndex && localDataBlockIndex < dataBlockIndexesPerBlock * dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	APIStatusCode result = SUCCESS;

	uint32_t metaDataBlockId = 0;
	uint32_t* tripleIndirectionDataBlockArray;
	if (localDataBlockIndex == 0) {
		result = acquireDataBlock(fileSystem, &metaDataBlockId);
		if (result == SUCCESS) {
			assert(metaDataBlockId != 0);
			*tripleIndirectionDataBlockId = metaDataBlockId;
			iNode->i_blocks += fileSystem->blockSize / 512;
			result = reserveBlockById(fileSystem, metaDataBlockId, (void**) &tripleIndirectionDataBlockArray);
			if (result == SUCCESS) {
				memset(tripleIndirectionDataBlockArray, 0, fileSystem->blockSize);
			}
		}

	} else {
		assert(*tripleIndirectionDataBlockId != 0);
		result = readAndReserveBlockById(fileSystem, *tripleIndirectionDataBlockId, (void**) &tripleIndirectionDataBlockArray);
	}
	if (result == SUCCESS) {
		result = appendDoubleIndirectionDataBlock(fileSystem, iNode,
				&tripleIndirectionDataBlockArray[localDataBlockIndex / (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock)],
				localDataBlockIndex % (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock),
				dataBlockId);
		releaseCachedBlockReservation(fileSystem, *tripleIndirectionDataBlockId, true);
	}
	if (result != SUCCESS && metaDataBlockId != 0) {
		iNode->i_blocks -= fileSystem->blockSize / 512;
		releaseDataBlock(fileSystem, metaDataBlockId);
	}

	return result;
}

static APIStatusCode appendDataBlockToInode(struct Ext2FileSystem* fileSystem, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, uint32_t dataBlockId) {
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	APIStatusCode result = SUCCESS;

	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	uint32_t size = localGetSize(fileSystem, iNode);
	uint32_t dataBlockIndex = mathUtilsCeilOfUint32Division(size, fileSystem->blockSize);

	if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT) {
		iNode->i_block[dataBlockIndex] = dataBlockId;
		iNode->i_blocks += fileSystem->blockSize / 512;

	} else if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT + dataBlockIndexesPerBlock) {
		uint32_t localDataBlockIndex = dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT;
		result = appendSingleIndirectionDataBlock(fileSystem, iNode, &iNode->i_block[EXT2_SINGLE_INDIRECTION_ENTRY_INDEX], localDataBlockIndex, dataBlockId);

	} else if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT + dataBlockIndexesPerBlock + dataBlockIndexesPerBlock * dataBlockIndexesPerBlock) {
		uint32_t localDataBlockIndex = dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT - dataBlockIndexesPerBlock;
		result = appendDoubleIndirectionDataBlock(fileSystem, iNode, &iNode->i_block[EXT2_DOUBLE_INDIRECTION_ENTRY_INDEX], localDataBlockIndex, dataBlockId);

	} else {
		uint32_t localDataBlockIndex = dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT - dataBlockIndexesPerBlock - (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		result = appendTripleIndirectionDataBlock(fileSystem, iNode, &iNode->i_block[EXT2_TRIPLE_INDIRECTION_ENTRY_INDEX], localDataBlockIndex, dataBlockId);
	}

	if (result == SUCCESS) {
		ext2VirtualFileSystemNode->isDirty = true;
	}
	return result;
}

static APIStatusCode removeSingleIndirectionDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t* singleIndirectionDataBlockId, uint32_t localDataBlockIndex) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	assert(*singleIndirectionDataBlockId > 0);
	assert(0 <= localDataBlockIndex && localDataBlockIndex < dataBlockIndexesPerBlock);

	APIStatusCode result = SUCCESS;

	uint32_t* singleIndirectionDataBlockArray;
	result = readAndReserveBlockById(fileSystem, *singleIndirectionDataBlockId, (void**) &singleIndirectionDataBlockArray);
	if (result == SUCCESS) {
		uint32_t dataBlockId = singleIndirectionDataBlockArray[localDataBlockIndex];
		/* Is it the last one? */
		if (localDataBlockIndex == 0) {
			releaseCachedBlockReservation(fileSystem, *singleIndirectionDataBlockId, false);
			releaseDataBlock(fileSystem, *singleIndirectionDataBlockId);
			*singleIndirectionDataBlockId = 0;
			iNode->i_blocks -= fileSystem->blockSize / 512;
		} else {
			singleIndirectionDataBlockArray[localDataBlockIndex] = 0;
			releaseCachedBlockReservation(fileSystem, *singleIndirectionDataBlockId, true);
		}
		releaseDataBlock(fileSystem, dataBlockId);
		iNode->i_blocks -= fileSystem->blockSize / 512;
	}

	return result;
}

static APIStatusCode removeDoubleIndirectionDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t* doubleIndirectionDataBlockId, uint32_t localDataBlockIndex) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	assert(*doubleIndirectionDataBlockId > 0);
	assert(0 <= localDataBlockIndex && localDataBlockIndex < dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	APIStatusCode result = SUCCESS;

	uint32_t* doubleIndirectionDataBlockArray;
	result = readAndReserveBlockById(fileSystem, *doubleIndirectionDataBlockId, (void**) &doubleIndirectionDataBlockArray);
	if (result == SUCCESS) {
		result = removeSingleIndirectionDataBlock(fileSystem, iNode,
				&doubleIndirectionDataBlockArray[localDataBlockIndex / dataBlockIndexesPerBlock],
				localDataBlockIndex % dataBlockIndexesPerBlock);
		/* Is it the last one? */
		if (localDataBlockIndex == 0) {
			releaseCachedBlockReservation(fileSystem, *doubleIndirectionDataBlockId, false);
			releaseDataBlock(fileSystem, *doubleIndirectionDataBlockId);
			*doubleIndirectionDataBlockId = 0;
			iNode->i_blocks -= fileSystem->blockSize / 512;
		} else {
			releaseCachedBlockReservation(fileSystem, *doubleIndirectionDataBlockId, true);
		}
	}

	return result;
}

static APIStatusCode removeTripleIndirectionDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, uint32_t* tripleIndirectionDataBlockId, uint32_t localDataBlockIndex) {
	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	assert(*tripleIndirectionDataBlockId > 0);
	assert(0 <= localDataBlockIndex && localDataBlockIndex < dataBlockIndexesPerBlock * dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);

	APIStatusCode result = SUCCESS;

	uint32_t* tripleIndirectionDataBlockArray;
	result = readAndReserveBlockById(fileSystem, *tripleIndirectionDataBlockId, (void**) &tripleIndirectionDataBlockArray);
	if (result == SUCCESS) {
		result = removeDoubleIndirectionDataBlock(fileSystem, iNode,
				&tripleIndirectionDataBlockArray[localDataBlockIndex / (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock)],
				localDataBlockIndex % (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock));
		/* Is it the last one? */
		if (localDataBlockIndex == 0) {
			releaseCachedBlockReservation(fileSystem, *tripleIndirectionDataBlockId, false);
			releaseDataBlock(fileSystem, *tripleIndirectionDataBlockId);
			*tripleIndirectionDataBlockId = 0;
			iNode->i_blocks -= fileSystem->blockSize / 512;
		} else {
			releaseCachedBlockReservation(fileSystem, *tripleIndirectionDataBlockId, true);
		}
	}

	return result;
}

static APIStatusCode removeLastDataBlock(struct Ext2FileSystem* fileSystem, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode) {
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;

	APIStatusCode result = SUCCESS;

	uint32_t dataBlockIndexesPerBlock = fileSystem->dataBlockIndexesPerBlock;
	uint32_t size = localGetSize(fileSystem, iNode);
	assert(size > 0);
	uint32_t dataBlockIndex = mathUtilsCeilOfUint32Division(size, fileSystem->blockSize) - 1;


	if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT) {
		uint32_t dataBlockId = iNode->i_block[dataBlockIndex];
		iNode->i_block[dataBlockIndex] = 0;
		releaseDataBlock(fileSystem, dataBlockId);
		iNode->i_blocks -= fileSystem->blockSize / 512;

	} else if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT + dataBlockIndexesPerBlock) {
		uint32_t localDataBlockIndex = dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT;
		result = removeSingleIndirectionDataBlock(fileSystem, iNode, &iNode->i_block[EXT2_SINGLE_INDIRECTION_ENTRY_INDEX], localDataBlockIndex);

	} else if (dataBlockIndex < EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT + dataBlockIndexesPerBlock + dataBlockIndexesPerBlock * dataBlockIndexesPerBlock) {
		uint32_t localDataBlockIndex = dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT - dataBlockIndexesPerBlock;
		result = removeDoubleIndirectionDataBlock(fileSystem, iNode, &iNode->i_block[EXT2_DOUBLE_INDIRECTION_ENTRY_INDEX], localDataBlockIndex);

	} else {
		uint32_t localDataBlockIndex = dataBlockIndex - EXT2_NO_INDIRECTION_DATA_BLOCKS_COUNT - dataBlockIndexesPerBlock - (dataBlockIndexesPerBlock * dataBlockIndexesPerBlock);
		result = removeTripleIndirectionDataBlock(fileSystem, iNode, &iNode->i_block[EXT2_TRIPLE_INDIRECTION_ENTRY_INDEX], localDataBlockIndex);
	}
	iNode->i_size = size - (size % fileSystem->blockSize == 0 ? fileSystem->blockSize : size % fileSystem->blockSize);
	ext2VirtualFileSystemNode->isDirty = true;
	return result;
}

static APIStatusCode writeSuperBlocksAndBlockGroupTables(struct Ext2FileSystem* fileSystem) {
	struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;

	APIStatusCode result = SUCCESS;

	int i = 0;
	struct DoubleLinkedListElement* doubleLinkedListElement1 = doubleLinkedListFirst(&fileSystem->blockGroupDescriptorsPageFrameList);
	while (result == SUCCESS && doubleLinkedListElement1 != NULL) {
		for (int j = 0; result == SUCCESS && j < fileSystem->blockGroupDescriptorsPerBlock && i < fileSystem->blockGroupsCount; j++) {
			void* data;
			if (i <= 1 || mathUtilsIsPowerOf(i, 3) || mathUtilsIsPowerOf(i, 5) || mathUtilsIsPowerOf(i, 7)) {
				/* Write the super block. */
				uint32_t blockId = i * superBlock->s_blocks_per_group + superBlock->s_first_data_block;
				result = readAndReserveBlockById(fileSystem, blockId, &data);
				if (result == SUCCESS) {
					struct Ext2SuperBlock* superBlock;
					if (i == 0 && blockId == 0) {
						superBlock = data + EXT2_SUPERBLOCK_OFFSET;
					} else {
						superBlock = data;
					}
					memcpy(superBlock, &fileSystem->superBlock, sizeof(struct Ext2SuperBlock));
					releaseCachedBlockReservation(fileSystem, blockId, true);
				}

				/* Write the block group table. */
				struct DoubleLinkedListElement* doubleLinkedListElement2 = doubleLinkedListFirst(&fileSystem->blockGroupDescriptorsPageFrameList);
				while (result == SUCCESS && doubleLinkedListElement2 != NULL) {
					blockId++;
					result = readAndReserveBlockById(fileSystem, blockId, &data);
					if (result == SUCCESS) {
						memcpy(data, (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement2), fileSystem->blockSize);
					}
					releaseCachedBlockReservation(fileSystem, blockId, true);

					doubleLinkedListElement2 = doubleLinkedListElement2->next;
				}

			}
			i++;
		}

		doubleLinkedListElement1 = doubleLinkedListElement1->next;
	}

	return result;
}

static APIStatusCode doWrite(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, off_t* offset,
		void* buffer, size_t ammountToWrite, size_t* count, bool* atLeastOneWriteSucceeded) {
	APIStatusCode result = SUCCESS;

	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;

	off_t localOffset = *offset;
	size_t size = localGetSize(fileSystem, iNode);

	while (result == SUCCESS && ammountToWrite > 0) {
		uint32_t appendedDataBlockId = 0;
		uint32_t dataBlockIndex;

		if (buffer == NULL) {
		}

		/* Do we need another block? */
		if (size == 0 || localOffset / fileSystem->blockSize > (size - 1) / fileSystem->blockSize) {
			result = acquireDataBlock(fileSystem, &appendedDataBlockId);
			if (result == SUCCESS) {
				result = appendDataBlockToInode(fileSystem, ext2VirtualFileSystemNode, appendedDataBlockId);
				if (result != SUCCESS) {
					releaseDataBlock(fileSystem, appendedDataBlockId);
				}
			}
			if (buffer == NULL) {
			}

		} else {
			appendedDataBlockId = 0;
		}

		if (result == SUCCESS) {
			dataBlockIndex = localOffset / fileSystem->blockSize;

			int localCount = fileSystem->blockSize - (localOffset % fileSystem->blockSize);
			localCount = mathUtilsMin(localCount, ammountToWrite);
			assert(localCount <= fileSystem->blockSize);
			assert((localCount + localOffset - 1) / fileSystem->blockSize == dataBlockIndex);
			assert((localOffset % fileSystem->blockSize) + localCount <= fileSystem->blockSize);

			uint32_t dataBlockId;
			void* data;
			if (appendedDataBlockId != 0) {
				result = reserveBlockById(fileSystem, appendedDataBlockId, &data);
				dataBlockId = appendedDataBlockId;
			} else {
				result = readInodeDataBlock(fileSystem, iNode, dataBlockIndex, &data, &dataBlockId);
			}
			if (result == SUCCESS) {
				assert(dataBlockId != 0);
				assert(appendedDataBlockId == 0 || dataBlockId == appendedDataBlockId);

				if (buffer != NULL) {
					memcpy(data + (localOffset % fileSystem->blockSize), buffer, localCount);
				} else {
					memset(data + (localOffset % fileSystem->blockSize), 0, localCount);
				}
				releaseCachedBlockReservation(fileSystem, dataBlockId, true);
				if (localOffset + localCount > size) {
					size = localOffset + localCount;
					iNode->i_size = size;
				}
				localOffset += localCount;
				ammountToWrite -= localCount;
				if (buffer != NULL) {
					buffer += localCount;
				}
				if (count != NULL) {
					*count += localCount;
				}
				if (atLeastOneWriteSucceeded != NULL) {
					*atLeastOneWriteSucceeded = true;
				}
			}
		}
	}

	*offset = localOffset;

	return result;
}

static APIStatusCode contextAwareWrite(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct OpenFileDescription* openFileDescription,
		void* buffer, size_t bufferSize, size_t* count) {
	APIStatusCode result = SUCCESS;

	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = context->fileSystem;

	if (S_ISREG(iNode->i_mode)) {
		bool atLeastOneWriteSucceeded = false;
		size_t size = localGetSize(fileSystem, iNode);
		*count = 0;

		size_t sizeOfGapToFill;
		if (openFileDescription->offset > size) {
			sizeOfGapToFill = openFileDescription->offset - size;
		} else {
			sizeOfGapToFill = 0;
		}

		ssize_t adjustedSizeOfGapToFill = -1;
		ssize_t adjustedBufferSize = -1;

		if (canIncreaseSize(size, sizeOfGapToFill)) {
			adjustedSizeOfGapToFill = sizeOfGapToFill;
			size_t sizeIncrement = sizeOfGapToFill;
			if (size > openFileDescription->offset) {
				if (bufferSize > size - openFileDescription->offset) {
					sizeIncrement += bufferSize - (size - openFileDescription->offset);
				}

			} else {
				sizeIncrement += bufferSize;
			}

			if (canIncreaseSize(size, sizeIncrement)) {
				adjustedBufferSize = bufferSize;
			} else {
				adjustedBufferSize = FILE_MAX_SIZE - size - sizeOfGapToFill;
			}
			assert (adjustedBufferSize <= ((ssize_t) bufferSize));

		} else {
			adjustedSizeOfGapToFill = FILE_MAX_SIZE - size;
		}
		assert(adjustedSizeOfGapToFill <= ((ssize_t) sizeOfGapToFill));

		if (adjustedSizeOfGapToFill > 0 || adjustedBufferSize > 0) {
			off_t offset;
			/* Fill the gap using zeros. */
			if (adjustedSizeOfGapToFill > 0) {
				offset = size;
				result = doWrite(ext2VirtualFileSystemNode, &offset, NULL, adjustedSizeOfGapToFill, NULL, &atLeastOneWriteSucceeded);
				assert(sizeOfGapToFill > adjustedSizeOfGapToFill || offset == openFileDescription->offset);
			}
			if (result == SUCCESS && adjustedBufferSize > 0) {
				assert(sizeOfGapToFill == adjustedSizeOfGapToFill);
				assert(localGetSize(fileSystem, iNode) >= openFileDescription->offset);

				offset = openFileDescription->offset;
				result = doWrite(ext2VirtualFileSystemNode, &offset, buffer, adjustedBufferSize, count, &atLeastOneWriteSucceeded);
			}

			openFileDescription->offset = offset;

		} else {
			result = EFBIG;
		}

		if (atLeastOneWriteSucceeded) {
			iNode->i_mtime = getUnixTime(context);
			ext2VirtualFileSystemNode->isDirty = true;
		}

	} else {
		result = EPERM;
	}

	return result;
}

static APIStatusCode write(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription,
		void* buffer, size_t bufferSize, size_t* count) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareWrite(&context, ext2VirtualFileSystemNode, openFileDescription, buffer, bufferSize, count);
}

static APIStatusCode insertINodeIntoDirectory(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, uint32_t iNodeIndex,
		const char* name, size_t nameLength, uint8_t fileType, struct Ext2LinkedDirectoryEntry** outputLinkedDirectoryEntry, uint32_t* outputDataBlockId) {
	assert((outputLinkedDirectoryEntry == NULL && outputDataBlockId == NULL) || (outputLinkedDirectoryEntry != NULL && outputDataBlockId != NULL));
	assert(nameLength <= FILE_NAME_MAX_LENGTH - 1);

	APIStatusCode result = SUCCESS;

	struct Ext2FileSystem* fileSystem = context->fileSystem;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	uint32_t size = localGetSize(fileSystem, iNode);
	size_t newLinkedDirectoryEntryMinimumSize = calculateMinimumLinkedDirectoryEntrySize(nameLength);

	uint32_t dataBlockId = 0;
	off_t offset = 0;
	void* data;
	struct Ext2LinkedDirectoryEntry* newLinkedDirectoryEntry = NULL;

	while (offset < size) {
		uint32_t dataBlockIndex = offset / fileSystem->blockSize;
		result = readInodeDataBlock(fileSystem, iNode, dataBlockIndex, &data, &dataBlockId);
		if (result == SUCCESS) {
			struct Ext2LinkedDirectoryEntry* currentLinkedDirectoryEntry = data + (offset % fileSystem->blockSize);

			if (currentLinkedDirectoryEntry->inode == 0 && currentLinkedDirectoryEntry->rec_len >= newLinkedDirectoryEntryMinimumSize) {
				newLinkedDirectoryEntry = currentLinkedDirectoryEntry;

			} else {
				size_t currentLinkedDirectoryEntryMinimumSize = calculateMinimumLinkedDirectoryEntrySize(currentLinkedDirectoryEntry->name_len);
				if (currentLinkedDirectoryEntryMinimumSize + newLinkedDirectoryEntryMinimumSize <= currentLinkedDirectoryEntry->rec_len) {
					newLinkedDirectoryEntry = ((void*) currentLinkedDirectoryEntry) + currentLinkedDirectoryEntryMinimumSize;
					newLinkedDirectoryEntry->rec_len = currentLinkedDirectoryEntry->rec_len - currentLinkedDirectoryEntryMinimumSize;
					currentLinkedDirectoryEntry->rec_len = currentLinkedDirectoryEntryMinimumSize;
					assert(data + (offset % fileSystem->blockSize) + currentLinkedDirectoryEntry->rec_len == newLinkedDirectoryEntry);
				}
			}

			if (newLinkedDirectoryEntry != NULL) {
				break;

			} else {
				releaseCachedBlockReservation(fileSystem, dataBlockId, false);
				offset += currentLinkedDirectoryEntry->rec_len;
			}

		} else {
			break;
		}
	}

	/* Do we need another data block? */
	if (result == SUCCESS && newLinkedDirectoryEntry == NULL) {
		assert(offset % fileSystem->blockSize == 0);

		if (canIncreaseSize(localGetSize(fileSystem, iNode), fileSystem->blockSize)) {
			result = acquireDataBlock(fileSystem, &dataBlockId);
			if (result == SUCCESS) {
				result = appendDataBlockToInode(fileSystem, ext2VirtualFileSystemNode, dataBlockId);
				if (result == SUCCESS) {
					iNode->i_size += fileSystem->blockSize;
					result = reserveBlockById(fileSystem, dataBlockId, &data);
					if (result == SUCCESS) {
						assert(fileSystem->blockSize >= newLinkedDirectoryEntryMinimumSize);
						newLinkedDirectoryEntry = data;
						newLinkedDirectoryEntry->rec_len = fileSystem->blockSize;

					} else {
						removeLastDataBlock(fileSystem, ext2VirtualFileSystemNode);
					}
				}

				if (result != SUCCESS) {
					releaseDataBlock(fileSystem, dataBlockId);
				}
			}

		} else {
			result = EFBIG;
		}
	}

	if (newLinkedDirectoryEntry != NULL) {
		assert(result == SUCCESS);

		iNode->i_ctime = getUnixTime(context);
		iNode->i_mtime = getUnixTime(context);
		ext2VirtualFileSystemNode->isDirty = true;

		memcpy(&newLinkedDirectoryEntry->nameFirstCharacter, name, nameLength);
		size_t unusedCharactersCount = newLinkedDirectoryEntry->rec_len - (sizeof(struct Ext2LinkedDirectoryEntry) - 1);
		assert(unusedCharactersCount >= nameLength);
		unusedCharactersCount -= nameLength;
		if (unusedCharactersCount >= 0) {
			/* Fill the remaining characters with zero. */
			memset(&newLinkedDirectoryEntry->nameFirstCharacter + nameLength, 0, unusedCharactersCount);
		}
		newLinkedDirectoryEntry->name_len = nameLength;
		newLinkedDirectoryEntry->inode = iNodeIndex;
		newLinkedDirectoryEntry->file_type = fileType;

		if (outputDataBlockId != NULL) {
			*outputDataBlockId = dataBlockId;
			*outputLinkedDirectoryEntry = newLinkedDirectoryEntry;

		} else {
			releaseCachedBlockReservation(fileSystem, dataBlockId, true);
		}

	} else {
		assert(result != SUCCESS);
	}

	return result;
}

static APIStatusCode readLinkedDirectoryEntry(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, const char* name, size_t nameLength,
		struct Ext2LinkedDirectoryEntry** linkedDirectoryEntry, uint32_t* dataBlockId) {
	assert(S_ISDIR(iNode->i_mode));

	int offset = 0;
	struct Ext2LinkedDirectoryEntry* localLinkedDirectoryEntry;
	uint32_t localDataBlockId;

	APIStatusCode result = SUCCESS;

	while (true) {
		bool endOfDirectory;
		result = readNextLinkedDirectoryEntry(fileSystem, iNode, &localLinkedDirectoryEntry, &offset, &localDataBlockId, &endOfDirectory);
		if (result == SUCCESS) {
			if (!endOfDirectory) {
				if (nameLength == localLinkedDirectoryEntry->name_len && strncmp(name, (char*) &localLinkedDirectoryEntry->nameFirstCharacter, nameLength) == 0) {
					*dataBlockId = localDataBlockId;
					*linkedDirectoryEntry = localLinkedDirectoryEntry;
					break;

				} else {
					releaseCachedBlockReservation(fileSystem, localDataBlockId, false);
				}

			} else {
				result = ENOENT;
				break;
			}

		} else {
			break;
		}
	}

	return result;
}

static APIStatusCode contextAwareWalk(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode,
		const char* pathSegment, bool createIfDoesNotExist, mode_t mode, struct VirtualFileSystemNode** nextVirtualFileSystemNode, bool* created) {
	assert(ext2VirtualFileSystemNode != NULL);
	assert(ext2VirtualFileSystemNode->virtualFileSystemNode.usageCount > 0);

	APIStatusCode result = SUCCESS;
	*nextVirtualFileSystemNode = NULL;

	if (created != NULL) {
		*created = false;
	}

	if (pathSegment == NULL) {
		result = SUCCESS;
		*nextVirtualFileSystemNode = &ext2VirtualFileSystemNode->virtualFileSystemNode;

	} else {
		size_t pathSegmentLength = strlen(pathSegment);
		assert(pathSegmentLength > 0);

		struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;

		if (S_ISDIR(ext2VirtualFileSystemNode->iNode.i_mode)) {
			struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
			uint32_t dataBlockId;

			result = readLinkedDirectoryEntry(fileSystem, &ext2VirtualFileSystemNode->iNode, pathSegment, pathSegmentLength, &linkedDirectoryEntry, &dataBlockId);
			if (result == SUCCESS) {
				result = getExt2VirtualFileSystemNodeByINodeIndex(ext2VirtualFileSystemNode->fileSystem, linkedDirectoryEntry->inode, (struct Ext2VirtualFileSystemNode**) nextVirtualFileSystemNode);
				if (result == SUCCESS) {
					(*nextVirtualFileSystemNode)->usageCount++;
				}
				releaseCachedBlockReservation(fileSystem, dataBlockId, false);
			}

			if (result == ENOENT && createIfDoesNotExist) {
				uint32_t iNodeIndex = 0;
				result = acquireINode(fileSystem, &iNodeIndex, false);
				if (result == SUCCESS) {
					assert(iNodeIndex != 0);

					struct Ext2LinkedDirectoryEntry* newLinkedDirectoryEntry = NULL;
					uint32_t linkedDirectoryEntryDataBlockId = 0;
					result = insertINodeIntoDirectory(context, ext2VirtualFileSystemNode, iNodeIndex, pathSegment, pathSegmentLength,
							EXT2_FT_REG_FILE, &newLinkedDirectoryEntry, &linkedDirectoryEntryDataBlockId);
					if (result == SUCCESS) {
						struct Ext2VirtualFileSystemNode* newExt2VirtualFileSystemNode;
						result = getExt2VirtualFileSystemNodeByINodeIndex(fileSystem, iNodeIndex, &newExt2VirtualFileSystemNode);
						if (result == SUCCESS) {
							assert(newExt2VirtualFileSystemNode->virtualFileSystemNode.usageCount == 0);

							struct Ext2INode* iNode = &newExt2VirtualFileSystemNode->iNode;
							memset(iNode, 0, sizeof(struct Ext2INode));
							iNode->i_atime = getUnixTime(context);
							iNode->i_ctime = getUnixTime(context);
							iNode->i_mode = EXT2_S_IFREG | mode;
							iNode->i_mtime = getUnixTime(context);
							iNode->i_links_count = 1;
							newExt2VirtualFileSystemNode->isDirty = true;
							newExt2VirtualFileSystemNode->virtualFileSystemNode.usageCount = 1;
							*nextVirtualFileSystemNode = (void*) newExt2VirtualFileSystemNode;

							if (created != NULL) {
								*created = true;
							}

						} else {
							newLinkedDirectoryEntry->inode = 0;
						}

						if (linkedDirectoryEntryDataBlockId != 0) {
							assert(newLinkedDirectoryEntry != NULL);
							releaseCachedBlockReservation(fileSystem, linkedDirectoryEntryDataBlockId, true);
						}
					}
				}

				if (result != SUCCESS && iNodeIndex != 0) {
					releaseINode(fileSystem, iNodeIndex, false);
				}
			}

		} else {
			result = ENOTDIR;
		}
	}

	return result;
}

static APIStatusCode walk(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process,
		const char* pathSegment, bool createIfDoesNotExist, mode_t mode, struct VirtualFileSystemNode** nextVirtualFileSystemNode, bool* created) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareWalk(&context, ext2VirtualFileSystemNode, pathSegment, createIfDoesNotExist, mode, nextVirtualFileSystemNode, created);
}

static APIStatusCode contextAwareChangeFileSize(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct OpenFileDescription* openFileDescription,
		off_t newSize) {
	APIStatusCode result = SUCCESS;

	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = context->fileSystem;

	uint32_t size = localGetSize(fileSystem, iNode);
	if (newSize > size) {
		off_t offset = size;

		uint32_t count = 0;
		bool atLeastOneWriteSucceeded = false;
		result = doWrite(ext2VirtualFileSystemNode, &offset, NULL, newSize - size, &count, &atLeastOneWriteSucceeded);
		if (result == SUCCESS) {
			assert(count == newSize - size);
			assert(localGetSize(fileSystem, iNode) == newSize);
		}

		if (atLeastOneWriteSucceeded) {
			iNode->i_ctime = getUnixTime(context);
			iNode->i_mtime = getUnixTime(context);
			ext2VirtualFileSystemNode->isDirty = true;
		}

	} else if (newSize < size) {
		uint32_t newDataBlockCount = mathUtilsCeilOfUint32Division(newSize, fileSystem->blockSize);
		uint32_t currentDataBlockCount = mathUtilsCeilOfUint32Division(size, fileSystem->blockSize);

		while (currentDataBlockCount > newDataBlockCount) {
			removeLastDataBlock(fileSystem, ext2VirtualFileSystemNode);
			currentDataBlockCount--;
		}
		iNode->i_size = newSize;
		ext2VirtualFileSystemNode->isDirty = true;

		iNode->i_mtime = getUnixTime(context);
	}

	return result;
}

static APIStatusCode changeFileSize(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process, struct OpenFileDescription* openFileDescription,
		off_t newSize) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareChangeFileSize(&context, ext2VirtualFileSystemNode, openFileDescription, newSize);
}

static APIStatusCode contextAwareCreateDirectory(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, const char* name, mode_t mode) {
	APIStatusCode result = SUCCESS;

	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = context->fileSystem;

	/* First we acquire all resources necessary. */
	uint32_t iNodeIndex = 0;
	uint32_t dataBlockId = 0;

	result = acquireINode(fileSystem, &iNodeIndex, true);
	if (result == SUCCESS) {
		assert(iNodeIndex != 0);
		result = acquireDataBlock(fileSystem, &dataBlockId);
		if (result == SUCCESS) {
			struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
			result = reserveBlockById(fileSystem, dataBlockId, (void**) &linkedDirectoryEntry);
			if (result == SUCCESS) {
				memset(linkedDirectoryEntry, 0, sizeof(struct Ext2LinkedDirectoryEntry));
				linkedDirectoryEntry->rec_len = (uint16_t) fileSystem->blockSize;
				releaseCachedBlockReservation(fileSystem, dataBlockId, true);
			}
		}
	}

	struct Ext2VirtualFileSystemNode* newExt2VirtualFileSystemNode = NULL;
	if (result == SUCCESS) {
		result = getExt2VirtualFileSystemNodeByINodeIndex(fileSystem, iNodeIndex, &newExt2VirtualFileSystemNode);
		assert(newExt2VirtualFileSystemNode->virtualFileSystemNode.usageCount == 0);
	}

	struct Ext2LinkedDirectoryEntry* newLinkedDirectoryEntry = NULL;
	uint32_t linkedDirectoryEntryDataBlockId = 0;
	if (result == SUCCESS) {
		assert(dataBlockId != 0);

		result = insertINodeIntoDirectory(context, ext2VirtualFileSystemNode, iNodeIndex, name, strlen(name), EXT2_FT_DIR,
				&newLinkedDirectoryEntry, &linkedDirectoryEntryDataBlockId);
		if (result == SUCCESS) {
			struct Ext2INode* newINode = &newExt2VirtualFileSystemNode->iNode;
			memset(newINode, 0, sizeof(struct Ext2INode));
			newINode->i_atime = getUnixTime(context);
			newINode->i_ctime = getUnixTime(context);
			newINode->i_mode = EXT2_S_IFDIR | mode;
			newINode->i_mtime = getUnixTime(context);
			newINode->i_links_count = 2;
			newINode->i_blocks = fileSystem->blockSize / 512;
			newINode->i_block[0] = dataBlockId;
			newINode->i_size = fileSystem->blockSize;
			newExt2VirtualFileSystemNode->isDirty = true;

			result = insertINodeIntoDirectory(context, newExt2VirtualFileSystemNode, iNodeIndex, ".", 1, EXT2_FT_DIR, NULL, NULL);
			if (result == SUCCESS) {
				result = insertINodeIntoDirectory(context, newExt2VirtualFileSystemNode, ext2VirtualFileSystemNode->iNodeIndex, "..", 2, EXT2_FT_DIR, NULL, NULL);
			}
			assert(getINodeBlockCount(fileSystem, newINode) == 1);

			contextAwareAfterNodeReservationRelease(context, newExt2VirtualFileSystemNode, NULL);

			if (result == SUCCESS) {
				iNode->i_atime = getUnixTime(context);
				iNode->i_mtime = getUnixTime(context);
				iNode->i_links_count++;
				ext2VirtualFileSystemNode->isDirty = true;
			}
		}
	}

	if (result != SUCCESS) {
		if (dataBlockId != 0) {
			releaseDataBlock(fileSystem, dataBlockId);
		}
		if (iNodeIndex != 0) {
			releaseINode(fileSystem, iNodeIndex, true);
		}
		if (newLinkedDirectoryEntry != NULL) {
			newLinkedDirectoryEntry->inode = 0;
		}
	}

	if (linkedDirectoryEntryDataBlockId != 0) {
		assert(newLinkedDirectoryEntry != NULL);
		releaseCachedBlockReservation(fileSystem, linkedDirectoryEntryDataBlockId, true);
	}

	return result;
}

static APIStatusCode createDirectory(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process, const char* name, mode_t mode) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareCreateDirectory(&context, ext2VirtualFileSystemNode, name, mode);
}

static APIStatusCode isDirectoryEmpty(struct Ext2FileSystem* fileSystem, struct Ext2INode* iNode, bool* isEmpty) {
	APIStatusCode result = SUCCESS;

	bool localIsEmpty = true;

	int offset = 0;
	while (localIsEmpty && result == SUCCESS) {
		struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
		uint32_t dataBlockId;
		bool endOfDirectory = false;

		result = readNextLinkedDirectoryEntry(fileSystem, iNode, &linkedDirectoryEntry, &offset, &dataBlockId, &endOfDirectory);
		if (result == SUCCESS) {
			if (endOfDirectory) {
				break;

			} else {
				if (!(1 == linkedDirectoryEntry->name_len && strncmp(".", &linkedDirectoryEntry->nameFirstCharacter, 1) == 0)
						&& !(2 == linkedDirectoryEntry->name_len && strncmp("..", &linkedDirectoryEntry->nameFirstCharacter, 2) == 0)) {
					localIsEmpty = false;
				}
				releaseCachedBlockReservation(fileSystem, dataBlockId, false);
			}
		}
	}

	*isEmpty = localIsEmpty;

	return result;
}

static APIStatusCode releaseDirectory(struct VirtualFileSystemNode* parentVirtualFileSystemNode, struct Process* process, struct VirtualFileSystemNode* directoryVirtualFileSystemNode) {
	APIStatusCode result = SUCCESS;

	struct Ext2VirtualFileSystemNode* parentExt2VirtualFileSystemNode = (void*) parentVirtualFileSystemNode;
	struct Ext2VirtualFileSystemNode* directoryExt2VirtualFileSystemNode = (void*) directoryVirtualFileSystemNode;
	struct Ext2INode* parentINode = &parentExt2VirtualFileSystemNode->iNode;
	struct Ext2INode* directoryINode = &directoryExt2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = parentExt2VirtualFileSystemNode->fileSystem;

	assert(S_ISDIR(parentINode->i_mode));
	assert(S_ISDIR(directoryINode->i_mode));

	if (directoryExt2VirtualFileSystemNode->iNodeIndex < fileSystem->superBlock.s_first_ino) {
		result = EINVAL;

	} else {
		bool isEmpty;
		result = isDirectoryEmpty(fileSystem, directoryINode, &isEmpty);
		if (result == SUCCESS) {
			if (isEmpty) {
				bool done = false;

				int offset = 0;
				while (!done && result == SUCCESS) {
					struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
					uint32_t dataBlockId;
					bool endOfDirectory = false;

					result = readNextLinkedDirectoryEntry(fileSystem, parentINode, &linkedDirectoryEntry, &offset, &dataBlockId, &endOfDirectory);
					if (result == SUCCESS) {
						bool modified = false;
						if (endOfDirectory) {
							assert(false);

						} else {
							if (linkedDirectoryEntry->inode == directoryExt2VirtualFileSystemNode->iNodeIndex) {
								assert(directoryINode->i_links_count == 2); /* As it is empty. */
								directoryINode->i_links_count = 0;
								assert(directoryExt2VirtualFileSystemNode->virtualFileSystemNode.usageCount == 1);
								directoryExt2VirtualFileSystemNode->virtualFileSystemNode.usageCount = 0;
								afterNodeReservationRelease(directoryExt2VirtualFileSystemNode, process, NULL);

								assert(linkedDirectoryEntry->file_type == EXT2_FT_DIR);
								parentINode->i_mtime = directoryINode->i_dtime;
								parentINode->i_ctime = directoryINode->i_dtime;
								linkedDirectoryEntry->inode = 0;
								assert(parentINode->i_links_count > 1);
								parentINode->i_links_count--;
								parentExt2VirtualFileSystemNode->isDirty = true;
								modified = true;
								done = true;
							}
							releaseCachedBlockReservation(fileSystem, dataBlockId, modified);
						}
					}
				}

			} else {
				result = ENOTEMPTY;
			}
		}
	}

	return result;
}

static uint8_t calculateFileType(mode_t mode) {
	if (S_ISREG(mode)) {
		return EXT2_FT_REG_FILE;
	} else if (S_ISDIR(mode)) {
		return EXT2_FT_DIR;
	} else if (S_ISLNK(mode)) {
		return EXT2_FT_SYMLINK;
	} else if (S_ISFIFO(mode)) {
		return EXT2_FT_FIFO;
	} else if (S_ISCHR(mode)) {
		return EXT2_FT_CHRDEV;
	} else if (S_ISBLK(mode)) {
		return EXT2_FT_BLKDEV;
	} else if (S_ISSOCK(mode)) {
		return EXT2_FT_SOCK;
	} else {
		return EXT2_FT_UNKNOWN;
	}
}

static APIStatusCode contextAwareCreateName(struct Context* context, struct Ext2VirtualFileSystemNode* ext2ParentVirtualFileSystemNode,
		struct Ext2VirtualFileSystemNode* ext2TargetVirtualFileSystemNode, const char* name, size_t nameLength) {
	APIStatusCode result = SUCCESS;

	struct Ext2FileSystem* fileSystem = context->fileSystem;
	assert(fileSystem == ext2ParentVirtualFileSystemNode->fileSystem);
	assert(fileSystem == ext2TargetVirtualFileSystemNode->fileSystem);

	struct Ext2INode* parentINode = &ext2ParentVirtualFileSystemNode->iNode;
	struct Ext2INode* targetINode = &ext2TargetVirtualFileSystemNode->iNode;

	assert(!S_ISDIR(targetINode->i_mode));
	assert(S_ISDIR(parentINode->i_mode));

	if (targetINode->i_links_count == UINT16_MAX) {
		result = EMLINK;

	} else {
		uint8_t fileType = calculateFileType(targetINode->i_mode);

		result = insertINodeIntoDirectory(context, ext2ParentVirtualFileSystemNode, ext2TargetVirtualFileSystemNode->iNodeIndex,
				name, nameLength, fileType, NULL, NULL);
		if (result == SUCCESS) {
			targetINode->i_ctime = getUnixTime(context);
			targetINode->i_links_count++;
			ext2TargetVirtualFileSystemNode->isDirty = true;
		}
	}

	return result;
}

static APIStatusCode createName(struct Ext2VirtualFileSystemNode* ext2ParentVirtualFileSystemNode, struct Process* process,
		struct Ext2VirtualFileSystemNode* ext2TargetVirtualFileSystemNode, const char* name) {
	struct Context context;
	initializeContext(&context, ext2ParentVirtualFileSystemNode->fileSystem, process);
	return contextAwareCreateName(&context, ext2ParentVirtualFileSystemNode, ext2TargetVirtualFileSystemNode, name, strlen(name));
}

static APIStatusCode contextAwareReleaseName(struct Context* context, struct Ext2VirtualFileSystemNode* parentExt2VirtualFileSystemNode,
		const char* name, size_t nameLength, bool verifyIfDirectory) {
	APIStatusCode result = SUCCESS;

	struct Ext2INode* parentINode = &parentExt2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = context->fileSystem;

	struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
	uint32_t dataBlockId;
	result = readLinkedDirectoryEntry(fileSystem, parentINode, name, nameLength, &linkedDirectoryEntry, &dataBlockId);
	if (result == SUCCESS) {
		bool modified = false;

		struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode;
		result = getExt2VirtualFileSystemNodeByINodeIndex(fileSystem, linkedDirectoryEntry->inode, &ext2VirtualFileSystemNode);
		if (result == SUCCESS) {
			struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
			if (!verifyIfDirectory || !S_ISDIR(iNode->i_mode)) {
				parentINode->i_mtime = getUnixTime(context);
				parentINode->i_ctime = getUnixTime(context);

				assert(linkedDirectoryEntry->inode == ext2VirtualFileSystemNode->iNodeIndex);
				linkedDirectoryEntry->inode = 0;
				assert(iNode->i_links_count > 0);
				iNode->i_links_count--;
				ext2VirtualFileSystemNode->isDirty = true;
				modified = true;

			} else {
				result = EISDIR;
			}

			contextAwareAfterNodeReservationRelease(context, ext2VirtualFileSystemNode, NULL);
		}

		releaseCachedBlockReservation(fileSystem, dataBlockId, modified);
	}

	return result;
}

static APIStatusCode releaseName(struct Ext2VirtualFileSystemNode* parentExt2VirtualFileSystemNode, struct Process* process, const char* name) {
	struct Context context;
	initializeContext(&context, parentExt2VirtualFileSystemNode->fileSystem, process);
	return contextAwareReleaseName(&context, parentExt2VirtualFileSystemNode, name, strlen(name), true);
}

static void contextAwareAfterNodeReservationRelease(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode,
		struct OpenFileDescription* openFileDescription) {
	if (ext2VirtualFileSystemNode->virtualFileSystemNode.usageCount == 0) {
		struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
		struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;

		APIStatusCode result = SUCCESS;
		if (iNode->i_links_count == 0) {
			if (!S_ISLNK(iNode->i_mode) || localGetSize(fileSystem, iNode) > EXT2_SYMBOLIC_LINK_SELF_CONTAINED_DATA_MAX_SIZE) {
				while (result == SUCCESS && localGetSize(fileSystem, iNode) > 0) {
					result = removeLastDataBlock(fileSystem, ext2VirtualFileSystemNode);
				}
			}
			if (result == SUCCESS) {
				iNode->i_dtime = getUnixTime(context);
				iNode->i_ctime = getUnixTime(context);
				ext2VirtualFileSystemNode->isDirty = true;
				releaseINode(fileSystem, ext2VirtualFileSystemNode->iNodeIndex, S_ISDIR(iNode->i_mode));
			}
		}

		if (result == SUCCESS && ext2VirtualFileSystemNode->isDirty) {
			result = writeINode(fileSystem, ext2VirtualFileSystemNode->iNodeIndex, &ext2VirtualFileSystemNode->iNode);
		}

		if (result != SUCCESS) {
			errorHandlerFatalError("There was a fatal error while trying to close a file or a directory", sys_errlist[result]);
		}

		enum OperationResult operationResult = bTreeRemove(&fileSystem->ext2VirtualFileSystemNodeByINodeIndex, &ext2VirtualFileSystemNode);
		assert(operationResult == B_TREE_SUCCESS);
		assert(!doubleLinkedListContainsFoward(&fileSystem->availableExt2VirtualFileSystemNodesList, &ext2VirtualFileSystemNode->availableListElement));
		doubleLinkedListInsertBeforeFirst(&fileSystem->availableExt2VirtualFileSystemNodesList, &ext2VirtualFileSystemNode->availableListElement);
	}
}

static void afterNodeReservationRelease(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* currentProcess,
		struct OpenFileDescription* openFileDescription) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, currentProcess);
	contextAwareAfterNodeReservationRelease(&context, ext2VirtualFileSystemNode, openFileDescription);
}

static APIStatusCode contextAwareCreateSymbolicLink(struct Context* context, struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode,
		const char* name, size_t nameLength, const char* targetPath, size_t targetPathLength) {
	APIStatusCode result = SUCCESS;

	struct Ext2FileSystem* fileSystem = context->fileSystem;

	/* First we acquire all resources necessary. */
	uint32_t iNodeIndex = 0;
	uint32_t dataBlockId = 0;

	void* data;
	result = acquireINode(fileSystem, &iNodeIndex, false);
	if (result == SUCCESS) {
		assert(iNodeIndex != 0);
		if (targetPathLength > EXT2_SYMBOLIC_LINK_SELF_CONTAINED_DATA_MAX_SIZE) {
			result = acquireDataBlock(fileSystem, &dataBlockId);
			if (result == SUCCESS) {
				result = reserveBlockById(fileSystem, dataBlockId, &data);
			}
		}
	}

	struct Ext2VirtualFileSystemNode* newExt2VirtualFileSystemNode = NULL;
	if (result == SUCCESS) {
		result = getExt2VirtualFileSystemNodeByINodeIndex(fileSystem, iNodeIndex, &newExt2VirtualFileSystemNode);
		assert(newExt2VirtualFileSystemNode->virtualFileSystemNode.usageCount == 0);
	}

	struct Ext2LinkedDirectoryEntry* newLinkedDirectoryEntry = NULL;
	uint32_t linkedDirectoryEntryDataBlockId = 0;
	if (result == SUCCESS) {
		assert(iNodeIndex != 0);
		result = insertINodeIntoDirectory(context, ext2VirtualFileSystemNode, iNodeIndex, name, strlen(name), EXT2_FT_SYMLINK, &newLinkedDirectoryEntry, &linkedDirectoryEntryDataBlockId);
		if (result == SUCCESS) {
			struct Ext2INode* newINode = &newExt2VirtualFileSystemNode->iNode;
			memset(newINode, 0, sizeof(struct Ext2INode));
			newINode->i_atime = getUnixTime(context);
			newINode->i_ctime= getUnixTime(context);
			newINode->i_mode = EXT2_S_IFLNK;
			newINode->i_mtime= getUnixTime(context);
			newINode->i_links_count = 1;
			newINode->i_size = targetPathLength;
			if (dataBlockId != 0) {
				newINode->i_blocks = fileSystem->blockSize / 512;
				newINode->i_block[0] = dataBlockId;
				memcpy(data, targetPath, targetPathLength);
			} else {
				memcpy(newINode->i_block, targetPath, targetPathLength);
			}
			newExt2VirtualFileSystemNode->isDirty = true;

			contextAwareAfterNodeReservationRelease(context, newExt2VirtualFileSystemNode, NULL);
		}
	}

	if (result != SUCCESS) {
		if (dataBlockId != 0) {
			releaseDataBlock(fileSystem, dataBlockId);
		}
		if (iNodeIndex != 0) {
			releaseINode(fileSystem, iNodeIndex, true);
		}
		if (newLinkedDirectoryEntry != NULL) {
			newLinkedDirectoryEntry->inode = 0;
		}
	}

	if (linkedDirectoryEntryDataBlockId != 0) {
		assert(newLinkedDirectoryEntry != NULL);
		releaseCachedBlockReservation(fileSystem, linkedDirectoryEntryDataBlockId, true);
	}

	return result;
}

static APIStatusCode createSymbolicLink(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode, struct Process* process,
		const char* name, const char* targetPath, size_t targetPathLength) {
	struct Context context;
	initializeContext(&context, ext2VirtualFileSystemNode->fileSystem, process);
	return contextAwareCreateSymbolicLink(&context, ext2VirtualFileSystemNode, name, strlen(name), targetPath, targetPathLength);
}

static APIStatusCode getDirectoryParent(struct VirtualFileSystemNode* virtualFileSystemNode, struct Process* process,
		struct VirtualFileSystemNode** virtualFileSystemNodeParent) {
	APIStatusCode result = SUCCESS;

	struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = (void*) virtualFileSystemNode;
	struct Ext2INode* iNode = &ext2VirtualFileSystemNode->iNode;
	struct Ext2FileSystem* fileSystem = ext2VirtualFileSystemNode->fileSystem;
	struct Ext2VirtualFileSystemNode* rootVirtualFileSystemNode = (void*) fileSystem->fileSystem.virtualFileSystemNode;

	if (ext2VirtualFileSystemNode != rootVirtualFileSystemNode) {
		struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
		uint32_t dataBlockId;
		result = readLinkedDirectoryEntry(fileSystem, iNode, "..", 2, &linkedDirectoryEntry, &dataBlockId);
		assert(result == SUCCESS);

		result = getExt2VirtualFileSystemNodeByINodeIndex(ext2VirtualFileSystemNode->fileSystem, linkedDirectoryEntry->inode,
				(struct Ext2VirtualFileSystemNode**) virtualFileSystemNodeParent);
		releaseCachedBlockReservation(fileSystem, dataBlockId, false);
		if (result == SUCCESS) {
			(*virtualFileSystemNodeParent)->usageCount++;
		}

	} else {
		*virtualFileSystemNodeParent = NULL;
	}

	return result;
}

static APIStatusCode contextAwareRename(struct Context* context,
		struct Ext2VirtualFileSystemNode* ext2TargetVirtualFileSystemNode,
		struct Ext2VirtualFileSystemNode* ext2ParentOfTargetVirtualFileSystemNode, const char* oldName,
		struct Ext2VirtualFileSystemNode* ext2ParentOfToBeReplacedVirtualFileSystemNode,
		struct Ext2VirtualFileSystemNode* ext2ToBeReplacedVirtualFileSystemNode, const char* newName) {
	APIStatusCode result = SUCCESS;

	struct Ext2INode* parentOfTargetINode = &ext2ParentOfTargetVirtualFileSystemNode->iNode;
	struct Ext2INode* targetINode = &ext2TargetVirtualFileSystemNode->iNode;
	struct Ext2INode* parentOfToBeReplacedINode = &ext2ParentOfToBeReplacedVirtualFileSystemNode->iNode;
	struct Ext2INode* toBeReplacedINode = NULL;
	if (ext2ToBeReplacedVirtualFileSystemNode != NULL) {
		toBeReplacedINode = &ext2ToBeReplacedVirtualFileSystemNode->iNode;
	}

	struct Ext2FileSystem* fileSystem = context->fileSystem;
	assert(fileSystem == ext2TargetVirtualFileSystemNode->fileSystem);
	assert(fileSystem == ext2ParentOfTargetVirtualFileSystemNode->fileSystem);
	assert(fileSystem == ext2ParentOfToBeReplacedVirtualFileSystemNode->fileSystem);
	assert(ext2ToBeReplacedVirtualFileSystemNode == NULL || fileSystem == ext2ToBeReplacedVirtualFileSystemNode->fileSystem);

	size_t newNameLenght = strlen(newName);
	size_t oldNameLength = strlen(oldName);

	uint8_t fileType = calculateFileType(targetINode->i_mode);

	if (ext2ToBeReplacedVirtualFileSystemNode == NULL) {
		/* Nothing will be replaced. */
		assert(S_ISDIR(parentOfToBeReplacedINode->i_mode));

		if (S_ISDIR(targetINode->i_mode) && parentOfToBeReplacedINode->i_links_count == UINT16_MAX) {
			/* As we need one more link for ".." entry and it does not have one available. */
			result = EMLINK;

		} else {
			result = insertINodeIntoDirectory(context, ext2ParentOfToBeReplacedVirtualFileSystemNode, ext2TargetVirtualFileSystemNode->iNodeIndex,
					newName, newNameLenght, fileType, NULL, NULL);

			if (result == SUCCESS) {
				result = contextAwareReleaseName(context, ext2ParentOfTargetVirtualFileSystemNode, oldName, oldNameLength, false);
				assert(result == SUCCESS);

				targetINode->i_ctime = getUnixTime(context);
				targetINode->i_links_count++; /* Don't need to check because there was already a link pointing to it that just got released. */
				ext2TargetVirtualFileSystemNode->isDirty = true;
			}
		}

	} else {
		/* Something will be replaced. */

		assert((S_ISDIR(targetINode->i_mode) && S_ISDIR(toBeReplacedINode->i_mode)) ^
				(!S_ISDIR(targetINode->i_mode) && !S_ISDIR(toBeReplacedINode->i_mode)));

		/* Is it a directory? */
		if (S_ISDIR(toBeReplacedINode->i_mode)) {
			bool isEmpty;
			result = isDirectoryEmpty(fileSystem, toBeReplacedINode, &isEmpty);
			if (result == SUCCESS && !isEmpty) {
				result = ENOTEMPTY;
			} else {
				toBeReplacedINode->i_links_count--; /* "." entry */
				parentOfToBeReplacedINode->i_links_count--; /* ".." reference */
			}
		}

		if (result == SUCCESS) {
			struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
			uint32_t dataBlockId;

			result = readLinkedDirectoryEntry(fileSystem, parentOfToBeReplacedINode, newName, newNameLenght, &linkedDirectoryEntry, &dataBlockId);
			assert(result == SUCCESS);

			assert(toBeReplacedINode->i_links_count > 0);
			toBeReplacedINode->i_links_count--;
			ext2ToBeReplacedVirtualFileSystemNode->isDirty = true;

			result = contextAwareReleaseName(context, ext2ParentOfTargetVirtualFileSystemNode, oldName, oldNameLength, false);
			assert(result == SUCCESS);

			targetINode->i_links_count++; /* Don't need to check because there was already a link pointing to it that just got released. */
			linkedDirectoryEntry->inode = ext2TargetVirtualFileSystemNode->iNodeIndex;
			linkedDirectoryEntry->file_type = fileType;
			ext2TargetVirtualFileSystemNode->isDirty = true;

			releaseCachedBlockReservation(fileSystem, dataBlockId, true);
		}
	}

	if (result == SUCCESS && S_ISDIR(targetINode->i_mode)) {
		assert(parentOfTargetINode->i_links_count > 0);
		parentOfTargetINode->i_links_count--; /* ".." reference */
		ext2ParentOfTargetVirtualFileSystemNode->isDirty = true;

		parentOfToBeReplacedINode->i_links_count++; /* ".." reference */
		ext2ParentOfToBeReplacedVirtualFileSystemNode->isDirty = true;

		/* Adjust ".." entry. */
		{
			struct Ext2LinkedDirectoryEntry* linkedDirectoryEntry;
			uint32_t dataBlockId;
			result = readLinkedDirectoryEntry(fileSystem, targetINode, "..", 2, &linkedDirectoryEntry, &dataBlockId);
			assert(result == SUCCESS);
			assert(linkedDirectoryEntry->file_type == EXT2_FT_DIR);
			linkedDirectoryEntry->inode = ext2ParentOfToBeReplacedVirtualFileSystemNode->iNodeIndex;
			releaseCachedBlockReservation(fileSystem, dataBlockId, true);
		}
	}

	return result;
}

static APIStatusCode afterUnmount(struct Ext2FileSystem* fileSystem) {
	APIStatusCode result = SUCCESS;

	struct Ext2VirtualFileSystemNode* rootVirtualFileSystemNode = (void*) fileSystem->fileSystem.virtualFileSystemNode;
	assert(rootVirtualFileSystemNode->virtualFileSystemNode.usageCount == 1);
	rootVirtualFileSystemNode->virtualFileSystemNode.usageCount--;
	afterNodeReservationRelease(rootVirtualFileSystemNode, NULL, NULL);
	assert(bTreeSize(&fileSystem->ext2VirtualFileSystemNodeByINodeIndex) == 0);

	if (fileSystem->isSuperBlockOrBlockGroupsDirty) {
		result = writeSuperBlocksAndBlockGroupTables(fileSystem);
		if (result != SUCCESS) {
			errorHandlerFatalError("There was a fatal error while writing file system metadata: %s", sys_errlist[result]);
		}
	}

	return result;
}

static APIStatusCode rename(struct Ext2VirtualFileSystemNode* ext2TargetVirtualFileSystemNode, struct Process* process,
		struct Ext2VirtualFileSystemNode* ext2ParentOfTargetVirtualFileSystemNode, const char* oldName,
		struct Ext2VirtualFileSystemNode* ext2ParentOfToBeReplacedVirtualFileSystemNode,
		struct Ext2VirtualFileSystemNode* ext2ToBeReplacedVirtualFileSystemNode, const char* newName) {
	struct Context context;
	initializeContext(&context, ext2TargetVirtualFileSystemNode->fileSystem, process);
	return contextAwareRename(&context, ext2TargetVirtualFileSystemNode,
		ext2ParentOfTargetVirtualFileSystemNode, oldName,
		ext2ParentOfToBeReplacedVirtualFileSystemNode,
		ext2ToBeReplacedVirtualFileSystemNode, newName);
}

static struct Ext2FileSystem* getFileSystem(struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode) {
	return ext2VirtualFileSystemNode->fileSystem;
}

APIStatusCode ext2FileSystemInitialize(struct Ext2FileSystem* fileSystem, struct BlockDevice* blockDevice, uint32_t maxSimultaneouslyOpenINodes) {
	struct VirtualFileSystemOperations* operations = &fileSystem->operations;
	memset(operations, 0, sizeof(struct VirtualFileSystemOperations));
	operations->open = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription**, int)) &open;
	operations->readDirectoryEntry = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, struct dirent*, bool*)) &readDirectoryEntry;
	operations->walk = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, const char*, bool, mode_t, struct VirtualFileSystemNode**, bool*)) &walk;
	operations->read = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, void*, size_t, size_t*)) &read;
	operations->write = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, void*, size_t, size_t*)) &write;
	operations->getOpenFileDescriptionOffsetRepositionPolicy = &getOpenFileDescriptionOffsetRepositionPolicy;
	operations->status = &status;
	operations->mergeWithSymbolicLinkPath = &mergeWithSymbolicLinkPath;
	operations->getMode = &getMode;
	operations->afterNodeReservationRelease = (void (*)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*)) &afterNodeReservationRelease;
	operations->changeFileSize = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct OpenFileDescription*, off_t)) &changeFileSize;
	operations->createDirectory = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, const char*, mode_t)) &createDirectory;
	operations->createName = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct VirtualFileSystemNode*, const char*)) &createName;
	operations->releaseName = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, const char*)) &releaseName;
	operations->releaseDirectory = &releaseDirectory;
	operations->createSymbolicLink = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, const char*, const char*, size_t)) &createSymbolicLink;
	operations->getSize = &getSize;
	operations->getDirectoryParent = &getDirectoryParent;
	operations->rename = (APIStatusCode (*)(struct VirtualFileSystemNode*, struct Process*, struct VirtualFileSystemNode*, const char*,
			struct VirtualFileSystemNode*, struct VirtualFileSystemNode*, const char*)) &rename;
	operations->getFileSystem = (struct FileSystem* (*)(struct VirtualFileSystemNode*)) &getFileSystem;

	fileSystem->blockDevice = blockDevice;
	doubleLinkedListInitialize(&fileSystem->blockGroupDescriptorsPageFrameList);

	fileSystem->isSuperBlockOrBlockGroupsDirty = false;

	APIStatusCode result;
	int worstCaseNodeCount = bTreeWorstCaseNodeCountToStore(PAGE_FRAME_SIZE, sizeof(void*), maxSimultaneouslyOpenINodes);
	logDebug("It will reserve %d page frames in order to store inode b-tree's meta data", worstCaseNodeCount);
	fileSystem->ext2VirtualFileSystemNodeByINodeIndexMemoryReservationId = memoryManagerReserveMemoryOnKernelSpace(worstCaseNodeCount);
	if (fileSystem->ext2VirtualFileSystemNodeByINodeIndexMemoryReservationId == -1) {
		result = ENOMEM;

	} else {
		bTreeInitialize(&fileSystem->ext2VirtualFileSystemNodeByINodeIndex, PAGE_FRAME_SIZE, sizeof(void*),
				(void*) fileSystem->ext2VirtualFileSystemNodeByINodeIndexMemoryReservationId,
				(void* (*)(void*, size_t)) &memoryAllocatorAcquire,
				(void (*)(void*, void*)) &memoryAllocatorRelease,
				(int (*)(const void*, const void*)) &compare
		);

		struct Ext2SuperBlock* superBlock = &fileSystem->superBlock;
		void* data;
		uint64_t blockId;
		result = blockCacheManagerReadAndReserveByOffset(blockDevice, EXT2_SUPERBLOCK_OFFSET, sizeof(struct Ext2SuperBlock) / blockDevice->blockSize, &data, &blockId);

		doubleLinkedListInitialize(&fileSystem->availableExt2VirtualFileSystemNodesList);
		doubleLinkedListInitialize(&fileSystem->ext2VirtualFileSystemNodesPageFrameList);
		while (result == SUCCESS && doubleLinkedListSize(&fileSystem->availableExt2VirtualFileSystemNodesList) < maxSimultaneouslyOpenINodes) {
			uint32_t ext2VirtualFileSystemNodesPerPageFrame = PAGE_FRAME_SIZE / sizeof(struct Ext2VirtualFileSystemNode);
			struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
			if (doubleLinkedListElement == NULL) {
				result = ENOMEM;

			} else {
				doubleLinkedListInsertAfterLast(&fileSystem->ext2VirtualFileSystemNodesPageFrameList, doubleLinkedListElement);
				struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNodes = (void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement);
				while (result == SUCCESS && ext2VirtualFileSystemNodesPerPageFrame > 0
						&& doubleLinkedListSize(&fileSystem->availableExt2VirtualFileSystemNodesList) < maxSimultaneouslyOpenINodes) {
					struct Ext2VirtualFileSystemNode* ext2VirtualFileSystemNode = &ext2VirtualFileSystemNodes[--ext2VirtualFileSystemNodesPerPageFrame];
					memset(ext2VirtualFileSystemNode, 0, sizeof(struct Ext2VirtualFileSystemNode));
					ext2VirtualFileSystemNode->fileSystem = fileSystem;
					doubleLinkedListInsertBeforeFirst(&fileSystem->availableExt2VirtualFileSystemNodesList, &ext2VirtualFileSystemNode->availableListElement);
				}
			}
		}

		if (result == SUCCESS) {
			memcpy(superBlock, data, sizeof(struct Ext2SuperBlock));
			blockCacheManageReleaseReservation(blockDevice, blockId, false);

			if (isValidExt2SuperBlock(superBlock) && isSupportedExt2FileSystem(superBlock) && superBlock->s_state == EXT2_VALID_FS) {
				fileSystem->blockSize = 1024 << superBlock->s_log_block_size;
				fileSystem->blockGroupsCount = mathUtilsCeilOfUint32Division(superBlock->s_blocks_count, superBlock->s_blocks_per_group);
				fileSystem->blockGroupDescriptorsPerBlock = fileSystem->blockSize / sizeof(struct Ext2BlockGroupDescriptor);
				fileSystem->iNodesPerBlock = fileSystem->blockSize / superBlock->s_inode_size;
				fileSystem->dataBlockIndexesPerBlock = fileSystem->blockSize / sizeof(uint32_t);

				/* Read and cache the content of "Block Group Descriptor Table". */
				uint64_t firstBlockIdForBlockGroupDescriptorsTable = EXT2_SUPERBLOCK_OFFSET / fileSystem->blockSize + 1;
				do {
					struct DoubleLinkedListElement* doubleLinkedListElement = memoryManagerAcquirePageFrame(true, -1);
					if (doubleLinkedListElement == NULL) {
						result = ENOMEM;
					} else {
						doubleLinkedListInsertAfterLast(&fileSystem->blockGroupDescriptorsPageFrameList, doubleLinkedListElement);
						void* data;
						result = readAndReserveBlockById(fileSystem, firstBlockIdForBlockGroupDescriptorsTable, &data);
						if (result == SUCCESS) {
							memcpy((void*) memoryManagerGetPageFramePhysicalAddress(doubleLinkedListElement), data, fileSystem->blockSize);
						}
						releaseCachedBlockReservation(fileSystem, firstBlockIdForBlockGroupDescriptorsTable, false);
						firstBlockIdForBlockGroupDescriptorsTable++;
					}
				} while (result == SUCCESS && fileSystem->blockGroupsCount > doubleLinkedListSize(&fileSystem->blockGroupDescriptorsPageFrameList) * fileSystem->blockGroupDescriptorsPerBlock);

				if (result == SUCCESS) {
					struct Ext2VirtualFileSystemNode* rootVirtualFileSystemNode;
					result = getExt2VirtualFileSystemNodeByINodeIndex(fileSystem, EXT2_ROOT_INO, &rootVirtualFileSystemNode);
					if (result == SUCCESS) {
						rootVirtualFileSystemNode->virtualFileSystemNode.usageCount++;
						fileSystem->fileSystem.virtualFileSystemNode = (void*) rootVirtualFileSystemNode;
						fileSystem->fileSystem.afterUnmount = (APIStatusCode (*)(struct FileSystem*)) &afterUnmount;
					}
				}

			} else {
				result = EINVAL;
			}
		}

		/* Release all memory in case of failure. */
		if (result != SUCCESS) {
			while (doubleLinkedListSize(&fileSystem->blockGroupDescriptorsPageFrameList) > 0) {
				struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&fileSystem->blockGroupDescriptorsPageFrameList);
				memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
			}

			while (doubleLinkedListSize(&fileSystem->ext2VirtualFileSystemNodesPageFrameList) > 0) {
				struct DoubleLinkedListElement* doubleLinkedListElement = doubleLinkedListRemoveFirst(&fileSystem->ext2VirtualFileSystemNodesPageFrameList);
				memoryManagerReleasePageFrame(doubleLinkedListElement, -1);
			}

			bTreeClear(&fileSystem->ext2VirtualFileSystemNodeByINodeIndex);
		}
	}

	return result;
}
