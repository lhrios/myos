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

#ifndef ASSERT_H
	#define ASSERT_H

	void doAssert(int condition, const char* format, ...);

	#ifndef NDEBUG
		#define assert(condition) \
			doAssert((condition) != 0, "Assertion test \"%s\" failed on file \"%s\" at line %d on function \"%s\"", \
				#condition, __FILE__, __LINE__, __func__)
	#else
		#define assert(condition)
	#endif
#endif
