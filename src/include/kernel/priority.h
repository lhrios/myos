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

#ifndef KERNEL_PRIORITY_H
	#define KERNEL_PRIORITY_H

	#include <limits.h>

	#define PRIORITY_HIGHEST LONG_MAX
	#define PRIORITY_VERY_HIGH ((LONG_MAX / 3) * 2)
	#define PRIORITY_HIGH ((LONG_MAX / 3) * 1)
	#define PRIORITY_MEDIUM 0
	#define PRIORITY_LOW ((LONG_MIN / 3) * 1)
	#define PRIORITY_VERY_LOW ((LONG_MIN / 3) * 2)
	#define PRIORITY_LOWEST LONG_MIN

#endif
