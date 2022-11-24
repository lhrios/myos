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

#include <stdint.h>

#include "kernel/api_status_code.h"

#include "kernel/io/block_device.h"

APIStatusCode blockCacheManagerPrintDebugReport(void);

APIStatusCode blockCacheManagerInitialize(uint32_t maxPageFramesToCacheBlocks);

APIStatusCode blockCacheManagerReserve(struct BlockDevice* blockDevice, uint64_t firstBlockId, uint32_t blockCount, void** data);
APIStatusCode blockCacheManagerReadAndReserve(struct BlockDevice* blockDevice, uint64_t firstBlockId, uint32_t blockCount, void** data);
APIStatusCode blockCacheManagerReadAndReserveByOffset(struct BlockDevice* blockDevice, uint32_t offset, uint32_t blockCount, void** data, uint64_t* firstBlockId);

APIStatusCode blockCacheManagerReadDirectly(struct BlockDevice* blockDevice, uint64_t firstBlockId, uint32_t blockCount, void*);

void blockCacheManageReleaseReservation(struct BlockDevice* blockDevice, uint64_t blockId, bool modified);

void blockCacheManageFlush(void);
void blockCacheManageClear(void);
