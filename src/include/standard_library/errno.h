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

#ifndef ERRNO_H
	#define ERRNO_H

	extern const char* const sys_errlist[];
	extern int sys_nerr;

	#ifndef KERNEL_CODE
		extern char* program_invocation_name;
   	extern char* program_invocation_short_name;

		extern int errno;
	#endif

	/*
	 * References:
	 * - https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
	 */

	/* Operation not permitted */
	#define EPERM 1

	/* No such file or directory */
	#define ENOENT 2

	/* No such process */
	#define ESRCH 3

	/* Interrupted system call */
	#define EINTR 4

	/* I/O error */
	#define EIO 5

	/* Arg list too long */
	#define E2BIG 7

	/* Executable format error */
	#define ENOEXEC 8

	/* Bad file descriptor */
	#define EBADF 9

	/* No child processes */
	#define ECHILD 10

	/* Try again */
	#define EAGAIN 11

	/* Out of memory */
	#define ENOMEM 12

	/* Permission denied */
	#define EACCES 13

	/* Bad address */
	#define EFAULT 14

	/* Block device required */
	#define ENOTBLK 15

	/* Device or resource busy */
	#define EBUSY 16

	/* File exists */
	#define EEXIST 17

	/* Cross-device link */
	#define EXDEV 18

	/* It is not a directory */
	#define ENOTDIR 20

	/* Is a directory */
	#define EISDIR 21

 	/* Invalid argument */
	#define EINVAL 22

	/* Too many open files */
	#define EMFILE 24

	/* Not a typewriter */
	#define ENOTTY 25

	/* File too large */
	#define EFBIG 27

	/* No space left on device */
	#define ENOSPC 28

	/* Illegal seek */
	#define ESPIPE 29

	/* Too many links */
	#define EMLINK 31

	/* Broken pipe */
	#define EPIPE 32

	/* Math argument out of domain of function */
	#define EDOM 33

	/* Math result not representable */
	#define ERANGE 34

	/* File name or path too long */
	#define ENAMETOOLONG 36

	/* Function not implemented */
	#define ENOSYS 38

	/* Directory not empty */
	#define ENOTEMPTY 39

	/* Too many symbolic links encountered */
	#define ELOOP 40

	/* Value too large for defined data type */
	#define EOVERFLOW 75

	/* Invalid or incomplete multibyte or wide character */
	#define EILSEQ 84

	/* Operation not supported */
	#define ENOTSUP 95

#endif
