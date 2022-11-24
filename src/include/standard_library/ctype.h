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

#ifndef CTYPE_H
	#define CTYPE_H

	static inline __attribute__((always_inline)) int isdigit(int c) {
		return '0' <= c && c <= '9';
	}

	static inline __attribute__((always_inline)) int isspace(int c) {
		return c == '\t' || c == ' ' || c == '\v' || c == '\n' || c == '\r' || c == '\f';
	}

	static inline __attribute__((always_inline)) int tolower(int c) {
   	if ('A' <= c && c <= 'Z') {
   		return c + ('a' - 'A');
   	} else {
   		return c;
   	}
   }

	static inline __attribute__((always_inline)) int toupper(int c) {
   	if ('a' <= c && c <= 'z') {
   		return c - ('a' - 'A');
   	} else {
   		return c;
   	}
   }

	static inline __attribute__((always_inline)) int islower(int c) {
   	return 'a' <= c && c <= 'z';
   }

	static inline __attribute__((always_inline)) int isupper(int c) {
   	return 'A' <= c && c <= 'Z';
   }

	static inline __attribute__((always_inline)) int isalpha(int c) {
		return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
	}

	static inline __attribute__((always_inline)) int isalnum(int c) {
   	return isalpha(c) || isdigit(c);
   }

	static inline __attribute__((always_inline)) int isxdigit(int c) {
   	return isdigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
	}

	static inline __attribute__((always_inline)) int isblank(int c) {
		return c == '\t' || c == ' ';
	}

	static inline __attribute__((always_inline)) int isprint(int c) {
		return ' ' <= c && c <= '\x7D';
	}

	int ispunct(int c);

	static inline __attribute__((always_inline)) int iscntrl(int c) {
		return ('\x00' <= c && c <= '\x1F') || c == '\x7F';
	}

	static inline __attribute__((always_inline)) int isgraph(int c) {
		return '!' <= c && c <= '~';
	}

	static inline __attribute__((always_inline)) int isascii(int c) {
		return c <= '\x7F';
	}

#endif
