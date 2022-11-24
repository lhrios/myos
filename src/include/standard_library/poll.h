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

#ifndef POLL_H
	#define POLL_H

	struct pollfd {
		int fd;
		short events;
		short revents;
	};

	#define POLLIN 0x0001 /* Data other than high-priority data may be read without blocking. */

	#define POLLERR 0x8000 /* An error has occurred (revents only). */
	#define POLLHUP 0x4000 /* Device has been disconnected (revents only). */
	#define POLLNVAL 0x2000 /* Invalid fd member (revents only). */

	typedef unsigned int nfds_t;

   int poll(struct pollfd* fds, nfds_t nfds, int timeout);

#endif
