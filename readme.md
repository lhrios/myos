# MyOS
MyOS is a hobbyist operating system project that targets the [x86 (32-bit) architecture](https://en.wikipedia.org/wiki/X86). It is composed by a [monolithic kernel](https://en.wikipedia.org/wiki/Monolithic_kernel) (written mainly in C programming language) and a [C standard library (libc)](https://en.wikipedia.org/wiki/C_standard_library) used by applications to access the functionalities provided by the kernel. The source-code is available under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html) license.

By being a Unix-like kernel, it's capable of suportting the execution of the following software*:
- [Gawk](https://www.gnu.org/software/gawk/) - version 3.1.8
- [GNU Bash](https://www.gnu.org/software/bash/) - version 5.1
- [GNU bc](https://www.gnu.org/software/bc/) - version 1.07.1 (and dc version 1.4.1)
- [GNU Binutils](https://www.gnu.org/software/binutils/) - version 2.37
- [GNU core utilities](https://www.gnu.org/software/coreutils/) - version 5.0
- [GNU Make](https://www.gnu.org/software/make/) - version 4.3
- [GNU Nano](https://www.nano-editor.org/) - version 5.5
- [GNU Readline Library](https://tiswww.case.edu/php/chet/readline/rltop.html) - version 8.1.2
- [ncurses library](https://invisible-island.net/ncurses/announce.html) - version 6.2

\* In some cases not all components or functionalities of the software are supported by MyOS kernel.		
		
## Building and running

In order to build and run MyOS, the following software is required:
- Bochs or Qemu
- GCC
- GRUB
- NASM

The commands above should be executed to build and run MyOS. Note some steps will require root access (to create and mount the file systems).
```sh
./create_hard_disk_image.sh
make qemu # Or "make bochs"
```

## References
The following material are the main references used throughout this project:
- [Advanced Programming in the UNIX Environment](https://en.wikipedia.org/wiki/Advanced_Programming_in_the_Unix_Environment)
- [ANSI escape code](https://en.wikipedia.org/wiki/ANSI_escape_code)
- [Linux manual pages: section 3](https://man7.org/linux/man-pages/dir_section_3.html)
- [OSDev.org](https://wiki.osdev.org/Main_Page)
- [The GNU C Library](https://www.gnu.org/software/libc/manual/)
- [The Open Group Library](https://publications.opengroup.org/)
