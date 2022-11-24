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

#ifndef UNISTD_H
	#define UNISTD_H

	#include <stdlib.h>

	#include <sys/types.h>

	#include "standard_library_implementation/file_descriptor_offset_reposition_constants.h"

	#define _POSIX_VERSION 200809L
	#define _POSIX2_C_VERSION 200809L

	#define STDIN_FILENO	0
	#define STDOUT_FILENO 1
	#define STDERR_FILENO 2

	extern char **environ;

	unsigned int sleep(unsigned int seconds);

	ssize_t read(int fileDescriptorIndex, void* buffer, size_t count);
	ssize_t write(int fileDescriptorIndex, const void* buffer, size_t count);
	off_t lseek(int fileDescriptorIndex, off_t offset, int whence);
	int close(int);

	pid_t fork(void);
	#define vfork() fork(); // TODO: Implement me!

	pid_t getpid(void);
	pid_t getppid(void);

	int truncate(const char* path, off_t newSize);
	int ftruncate(int fileDescriptorIndex, off_t newSize);

	int execv(const char* executablePath, char *const argv[]);
   int execve(const char* executablePath, char *const argv[], char *const envp[]);
	int execvpe(const char* executableNameOrPath, char* const argv[], char* const envp[]);
	int execvp(const char* executableNameOrPath, char* const argv[]);
	int execl(const char* executablePath, const char* arg, ...);

	char* getcwd(char* buffer, size_t bufferSize);
   int chdir(const char* path);

   int setuid(uid_t userId); // TODO: Implement me!
   uid_t getuid(void); // TODO: Implement me!
   uid_t geteuid(void); // TODO: Implement me!

   int setgid(gid_t groupdId); // TODO: Implement me!
   gid_t getgid(void); // TODO: Implement me!
   gid_t getegid(void); // TODO: Implement me!

   pid_t getpgrp(void);
   pid_t getpgid(pid_t processId);

   int setpgid(pid_t processId, pid_t processGroupId);
   int setpgrp(void);

   pid_t getsid(pid_t processId);
   pid_t setsid(void);

   int getopt(int argc, char * const argv[], const char *optstring);
   extern char* optarg;
   extern int opterr, optind, optopt;

   // TODO: Implement me!
	#define R_OK 1
	#define W_OK 2
	#define X_OK 3
	#define F_OK 4
   int access(const char* path, int mode); // TODO: Implement me!
   int euidaccess(const char* path, int mode); // TODO: Implement me!

   unsigned int alarm(unsigned int seconds); // TODO: Implement me!

   int gethostname(char* buffer, size_t bufferSize);

   int isatty(int fileDescriptorIndex);

   int tcsetpgrp(int fileDescriptorIndex, pid_t processGroupId);
   pid_t tcgetpgrp(int fileDescriptorIndex);

	#define _SC_CLK_TCK 1
	#define _SC_PAGESIZE 2
   long sysconf(int name);

	#define getpagesize() ((int) sysconf(_SC_PAGESIZE))

	#define _CS_PATH 1
   size_t confstr(int name, char* buffer, size_t bufferSize);

	#define _PC_PIPE_BUF 1
	#define _PC_PATH_MAX 2
   long fpathconf(int fileDescriptorIndex, int name);
   long pathconf(const char* path, int name);

   int pipe(int pipeFileDescriptorIndexes[2]);

   int dup(int existentFileDescriptorIndex);
   int dup2(int existentFileDescriptorIndex, int newFileDescriptorIndex);

   ssize_t readlink(const char* path, char* buffer, size_t bufferSize);
   int unlink(const char* namePathToRelease);
   int link(const char* targetPath, const char* namePathToCreate);
   int rmdir(const char* directoryPathToRelease);
   int symlink(const char* targetPath, const char* symbolicLinkPathToCreate);

   void* sbrk(int increment);

   void _exit(int status);

   ssize_t pread(int fileDescriptorIndex, void* buffer, size_t count, off_t offset); // TODO: Implement me!

   int getgroups(int size, gid_t groups[]); // TODO: Implement me!

#endif
