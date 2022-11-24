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

#ifndef ITERATOR_H
	#define ITERATOR_H

	#include <stdbool.h>

	struct Iterator {
		bool (*hasNext)(struct Iterator* iterator);
		void* (*next)(struct Iterator* iterator);
		void* (*transformValueBeforeNextReturn)(struct Iterator* iterator, void* value);
	};

	inline __attribute__((always_inline)) bool iteratorHasNext(struct Iterator* iterator) {
		return iterator->hasNext(iterator);
	}

	inline __attribute__((always_inline)) void* iteratorNext(struct Iterator* iterator) {
		if (iterator->transformValueBeforeNextReturn) {
			return iterator->transformValueBeforeNextReturn(iterator, iterator->next(iterator));
		} else {
			return iterator->next(iterator);
		}
	}

#endif
