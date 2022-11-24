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

#ifndef TERMIOS_H
	#define TERMIOS_H

	#include <sys/types.h>

	typedef unsigned int tcflag_t;
	typedef unsigned int	speed_t;
	typedef char cc_t;

	#define VSUSP 0
	#define VQUIT 1
	#define VINTR 2
	#define VKILL 3
	#define VEOF 4
	#define VEOL 5
	#define VERASE 6
	#define VWERASE 7
	#define NCCS 8
	struct termios {
		tcflag_t c_iflag; /* input modes */
		tcflag_t c_oflag; /* output modes */
		tcflag_t c_cflag; /* control modes */
		tcflag_t c_lflag; /* local modes */
		cc_t c_cc[NCCS]; /* special characters */
	};

   struct winsize {
		unsigned short ws_row;
		unsigned short ws_col;
      unsigned short ws_xpixel; /* unused */
      unsigned short ws_ypixel; /* unused */
   };

	/*
	 * References:
	 *
	 * - https://www.gnu.org/software/libc/manual/html_node/Mode-Functions.html
	 * - https://www.gnu.org/software/libc/manual/html_node/Line-Control.html
	 * - https://man7.org/linux/man-pages/man3/tcflush.3.html
	 */

	#define TCSANOW 1 /* Change attributes immediately. */
	#define TCSADRAIN 2 /* Make the change after waiting until all queued output has been written. */
   #define TCSAFLUSH 3 /* This is like TCSADRAIN, but also discards any queued input. */
   int tcsetattr(int fileDescriptorIndex, int optionalActions, const struct termios* termiosInstance);

	#define TCGETS 1
	#define TCSETS 2
	#define TCSETSW 3
	#define TCSETSF 4
	#define TIOCGWINSZ 5
	#define TIOCSWINSZ 9
	#define TIOCGPGRP 6
	#define TIOCSPGRP 7
	#define TIOCGSID 8

	#define TCIFLUSH 9 /* It flushes data received but not read. */
	#define TCOFLUSH 10 /* It flushes data written but not transmitted. */
	#define TCIOFLUSH 11 /* It flushes both data received but not read, and data written but not transmitted. */
	int tcflush(int fileDescriptorIndex, int queueSelector); // TODO: Implement me!

	/* c_lflag flag constants: */
	#define ICANON (1 << 0)
	#define ECHO (1 << 1)
	#define ISIG (1 << 2)
	#define NOFLSH (1 << 8)
	#define ECHOK (1 << 4)
	#define ECHOE (1 << 5)
	#define ECHONL (1 << 3)
	#define IEXTEN (1 << 6)
	#define TOSTOP (1 << 7)
	#define ECHOCTL (1 << 8)

	/* c_iflag flag constants: */
	#define ICRNL (1 << 2)
	#define INLCR (1 << 1)
	#define IGNCR (1 << 8)
	// TODO: Implement me!
	#define IXOFF (1 << 5)
	#define BRKINT (1 << 6)
	#define IGNBRK (1 << 10)
	#define IXON (1 << 4)
	/* Unnecessary given the context? */
	#define ISTRIP (1 << 0)
	#define INPCK (1 << 3)
	#define IGNPAR (1 << 7)
	#define PARMRK (1 << 9)

	/* c_cflag flag constants: */
	/* Unnecessary given the context? */
	#define CLOCAL (1 << 1)
	#define CREAD (1 << 2)
	#define CSTOPB (1 << 3)
	#define HUPCL (1 << 4)
	#define PARODD (1 << 5)
	#define PARENB (1 << 0)

	// TODO: Implement me!
	#define VSTART 0
	#define VSTOP 0
	#define VTIME 0
	#define VMIN 0

	/* c_oflag flag constants: */
	#define CSIZE 0x7
	#define CS5 (1 << 0)
	#define CS6 (2 << 0)
	#define CS7 (3 << 0)
	#define CS8 (4 << 0)
	// TODO: Implement me!
	#define OPOST (1 << 4)
	#define ONLCR (1 << 5)

	#define B0 0
	#define B50 1
	#define B75 2
	#define B110 3
	#define B134 4
	#define B150 5
	#define B200 6
	#define B300 7
	#define B600 8
	#define B1200 11
	#define B1800 12
	#define B2400 13
	#define B4800 14
	#define B9600 15
	#define B19200 16
	#define B38400 17
	#define B57600 18
	#define B115200 19
	#define B230400 20

	#define TCOOFF 0 /* suspends output. */
	#define TCOON 1 /* restarts suspended output. */
	#define TCIOFF 2 /* transmits a STOP character, which stops the terminal device from transmitting data to the system. */
	#define TCION 3 /* transmits a START character, which starts the terminal device transmitting data to the system. */
   int tcflow(int fileDescriptorIndex, int action);

   int tcsetattr(int fileDescriptorIndex, int optionalActions, const struct termios* termiosInstance);
	int tcgetattr(int fileDescriptorIndex, struct termios* termiosInstance);

	pid_t tcgetsid(int fileDescriptorIndex);

	speed_t cfgetispeed(const struct termios* termiosInstance);
	int cfsetispeed(struct termios* termiosInstance, speed_t speed);

	speed_t cfgetospeed(const struct termios* termiosInstance);
	int cfsetospeed(struct termios* termiosInstance, speed_t speed);

#endif
