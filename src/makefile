# makefile @ src folder

export

CC = gcc
CC_FLAGS = -c -g -O2 -m32 \
-Wno-main \
-Wno-address-of-packed-member \
-Wall \
$(if ${NDEBUG},-DNDEBUG,-DDEBUG) \
-undef \
-nostdinc \
-nostdlib \
-nodefaultlibs \
-nostartfiles \
-fno-builtin \
-fstrength-reduce \
-fno-stack-protector \
-fno-omit-frame-pointer \
-fno-asynchronous-unwind-tables \
-fno-exceptions \
-fno-unwind-tables \
-fno-toplevel-reorder \
-fno-inline \
-fno-pic \
-fcf-protection=none
# -nostdinc: Do not search the standard system directories for header files.
# -nostdlib: Do not use the standard system startup files or libraries when linking.
# -nodefaultlibs: Do not use the standard system libraries when linking.
# -nostartfiles: Do not use the standard system startup files when linking.
# -fno-builtin: Do not recognize built-in functions that do not begin with "__builtin_" as prefix.
# -fno-pic: Do not generate position-independent code (PIC) suitable for use in a shared library.
CPP_FLAGS =
LD = ld
LD_FLAGS = -m elf_i386 -T script.ld --orphan-handling=discard
NASM = nasm
NASM_FLAGS = -f elf
SED = sed
SHELL = /bin/bash
RM = rm
RM_FLAGS = -rf

target_path = ../target
bin_path_base := $(target_path)/bin
bin_path = $(bin_path_base)
dep_path = $(target_path)/dep
include_path = include
kernel_file = kernel
modules_list = common user/assembly user kernel/assembly kernel test/integration
src_path = $(target_path)/src/

.PHONY: all
all : $(src_path) $(dep_path) $(bin_path) $(modules_list)

.PHONY: test
test:
	./compile_and_run_tests.sh

# The sed regular expression substitutes the "test/integration" by "../..".
.PHONY: $(modules_list)
$(modules_list) :
	$(eval compilation_root_path := $(shell echo '$@' | sed 's/[^/]*/\.\./g'))
	$(MAKE) -RI $(compilation_root_path)/$(dep_path)/$@ -C $@

.PHONY: clean
clean :
	$(RM) $(RM_FLAGS) $(target_path)

.PHONY: todo
todo :
	@grep -r TODO
	@cat TODO

.PHONY: dump
dump : $(bin_path)/kernel/$(kernel_file)
	objdump -d -S $< > /tmp/dump

.ONESHELL:    
$(dep_path) :
	for module in $(modules_list); do
		for submodule in `find $$module -type d`; do
			mkdir -p $(@)/$$submodule;
		done;
	done;

.ONESHELL:
$(bin_path) :
	for module in $(modules_list); do
		for submodule in `find $$module -type d`; do
			mkdir -p $(@)/$$submodule;
		done;
	done;

.ONESHELL:
$(src_path) :
	mkdir -p $(@)
