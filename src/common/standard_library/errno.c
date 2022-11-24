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

const char* const sys_errlist[] = {
	"", /* 0 */
	"Operation not permitted", /* 1 */
	"No such file or directory", /* 2 */
	"No such process", /* 3 */
	"Interrupted system call", /* 4 */
	"I/O error", /* 5 */
	"", /* 6 */
	"Argument and/or environment parameter list too long", /* 7 */
	"Executable format error", /* 8 */
	"Bad file descriptor", /* 9 */

	"No child processes", /* 10 */
	"Try again", /* 11 */
	"Out of memory", /* 12 */
	"Permission denied", /* 13 */
	"Bad address", /* 14 */
	"Block device required", /* 15 */
	"Device or resource busy", /* 16 */
	"File exists", /* 17 */
	"", /* 18 */
	"", /* 19 */

	"It is not a directory", /* 20 */
	"Is a directory", /* 21 */
	"Invalid argument", /* 22 */
	"", /* 23 */
	"Too many open files", /* 24 */
	"Not a typewriter", /* 25 */
	"", /* 26 */
	"File too large", /* 27 */
	"No space left on device", /* 28 */
	"Illegal seek", /* 29 */

	"", /* 30 */
	"Too many links", /* 31 */
	"Broken pipe", /* 32 */
	"Math argument out of domain of function", /* 33 */
	"Math result not representable", /* 34 */
	"", /* 35 */
	"File name or path too long", /* 36 */
	"", /* 37 */
	"Function not implemented", /* 38 */
	"Directory not empty", /* 39 */

	"Too many symbolic links encountered", /* 40 */
	"", /* 41 */
	"", /* 42 */
	"", /* 43 */
	"", /* 44 */
	"", /* 45 */
	"", /* 46 */
	"", /* 47 */
	"", /* 48 */
	"", /* 49 */

	"", /* 50 */
	"", /* 51 */
	"", /* 52 */
	"", /* 53 */
	"", /* 54 */
	"", /* 55 */
	"", /* 56 */
	"", /* 57 */
	"", /* 58 */
	"", /* 59 */

	"", /* 60 */
	"", /* 61 */
	"", /* 62 */
	"", /* 63 */
	"", /* 64 */
	"", /* 65 */
	"", /* 66 */
	"", /* 67 */
	"", /* 68 */
	"", /* 69 */

	"", /* 70 */
	"", /* 71 */
	"", /* 72 */
	"", /* 73 */
	"", /* 74 */
	"Value too large for defined data type", /* 75 */
	"", /* 76 */
	"", /* 77 */
	"", /* 78 */
	"" /* 79 */

	"", /* 80 */
	"", /* 81 */
	"", /* 82 */
	"", /* 83 */
	"Invalid or incomplete multibyte or wide character", /* 84 */
	"", /* 85 */
	"", /* 86 */
	"", /* 87 */
	"", /* 88 */
	"" /* 89 */

	"", /* 90 */
	"", /* 91 */
	"", /* 92 */
	"", /* 93 */
	"", /* 94 */
	"Operation not supported", /* 95 */
	"", /* 96 */
	"", /* 97 */
	"", /* 98 */
	"" /* 99 */
};
int sys_nerr = (sizeof(sys_errlist) / sizeof(const char*));
