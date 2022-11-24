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

#ifndef KERNEL_SYSTEM_CALL_H
	#define KERNEL_SYSTEM_CALL_H

	#define INTERRUPTION_VECTOR_TO_HANDLE_SYSTEM_CALL 200

	#define SYSTEM_CALL_INEXISTENT 0x00
	#define SYSTEM_CALL_SLEEP 0x01
	#define SYSTEM_CALL_EXIT 0x02
	#define SYSTEM_CALL_GET_PROCESS_ID 0x03
	#define SYSTEM_CALL_GET_PARENT_PROCESS_ID 0x04
	#define SYSTEM_CALL_FORK 0x05
	#define SYSTEM_CALL_MONITOR_IO_EVENTS 0x06
	#define SYSTEM_CALL_OPEN 0x07
	#define SYSTEM_CALL_READ 0x08
	#define SYSTEM_CALL_WRITE 0x09
	#define SYSTEM_CALL_CLOSE 0xA
	#define SYSTEM_CALL_WAIT 0x0B
	#define SYSTEM_CALL_CHANGE_DATA_SEGMENT_SIZE 0x0C
	#define SYSTEM_CALL_GET_UNIX_TIME 0x0D
	#define SYSTEM_CALL_STATUS 0x0E
	#define SYSTEM_CALL_READ_DIRECTORY_ENTRY 0x0F
	#define SYSTEM_CALL_EXECUTE_EXECUTABLE 0x10
	#define SYSTEM_CALL_REPOSITION_OPEN_FILE_DESCRIPTION_OFFSET 0x11
	#define SYSTEM_CALL_CHANGE_SIGNAL_ACTION 0x12
	#define SYSTEM_CALL_GENERATE_SIGNAL 0x13
	#define SYSTEM_CALL_CHANGE_SIGNALS_BLOCKAGE 0x14
	#define SYSTEM_CALL_GET_CURRENT_WORKING_DIRECTORY 0x15
	#define SYSTEM_CALL_SET_CURRENT_WORKING_DIRECTORY 0x16
	#define SYSTEM_CALL_CACHE_FLUSH_AND_CLEAR 0x17
	#define SYSTEM_CALL_CACHE_FLUSH 0x18
	#define SYSTEM_CALL_REBOOT 0x19
	#define SYSTEM_CALL_CHANGE_FILE_SIZE 0x1A
	#define SYSTEM_CALL_CREATE_DIRECTORY 0x1B
	#define SYSTEM_CALL_RELEASE_NAME 0x1C
	#define SYSTEM_CALL_CREATE_NAME 0x1D
	#define SYSTEM_CALL_RELEASE_DIRECTORY 0x1E
	#define SYSTEM_CALL_CREATE_SYMBOLIC_LINK 0x1F
	#define SYSTEM_CALL_CHANGE_DEVICE_PARAMETERS 0x20
	#define SYSTEM_CALL_CHANGE_OPEN_FILE_DESCRIPTION_PARAMETERS 0x21
	#define SYSTEM_CALL_DUPLICATE_FILE_DESCRIPTOR 0x22
	#define SYSTEM_CALL_CREATE_SESSION_AND_PROCESS_GROUP 0x23
	#define SYSTEM_CALL_GET_SESSION_ID 0x24
	#define SYSTEM_CALL_SET_PROCESS_GROUP 0x25
	#define SYSTEM_CALL_GET_PROCESS_GROUP_ID 0x26
	#define SYSTEM_CALL_CREATE_PIPE 0x27
	#define SYSTEM_CALL_SET_FILE_MODE_CREATION_MASK 0x28
	#define SYSTEM_CALL_RENAME 0x29
	#define SYSTEM_CALL_CHANGE_FILE_DESCRIPTOR_PARAMETERS 0x30

	#define SYSTEM_CALL_ASSERT_FALSE 0xD0
	#define SYSTEM_CALL_BUSY_WAIT 0xD1
	#define SYSTEM_CALL_FORCE_KERNEL_SIGSEGV 0xD2
	#define SYSTEM_CALL_FORCE_KERNEL_SIGILL 0xD3
	#define SYSTEM_CALL_LOG_KERNEL_MODULE_DEBUG_REPORT 0xD4
	#define SYSTEM_CALL_GET_PROCESS_MEMORY_SEGMENTS_LIMITS 0xD5

#endif