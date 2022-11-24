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

#ifndef PIPE_H
	#define PIPE_H

	#include "kernel/api_status_code.h"
	#include "kernel/process/process.h"

	#include "kernel/io/open_file_description.h"

	APIStatusCode pipeManagerCreatePipe(struct Process* currentProcess, int* readFileDescriptorIndex, int* writeFileDescriptorIndex);
	APIStatusCode pipeManagerInitialize(void);

#endif
