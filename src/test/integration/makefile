# makefile @ test/integration

dep_path := ../../$(dep_path)/test/integration
bin_path_base := ../../$(bin_path_base)
bin_path := ../../$(bin_path)/test/integration
standard_library := $(bin_path_base)/user/standard_library/standard_library.a
include_path := ../$(include_path)

executables += $(foreach file, \
	$(shell find . -iname 'test_*.c'), \
	$(patsubst %.c,%,$(file)))
executables += $(foreach file, \
	$(shell find . -iname 'executable_*.c'), \
	$(patsubst %.c,%,$(file)))

include ../../c_general_makefile
include ../../general_makefile

$(executables) : % : %.o $(standard_library) executable_entry_point_asm.o integration_test.o makefile
	@echo creating $(@F)
	@ld -m elf_i386 -T ../../user/script.ld --orphan-handling=discard \
		$(bin_path_base)/user/assembly/executable_entry_point_asm.o \
		$(bin_path)/$(<F) \
		$(bin_path)/integration_test.o \
		$(standard_library) \
		--oformat=binary -o $(bin_path)/executables/$(@F)

CC_FLAGS += -fno-zero-initialized-in-bss

$(shell mkdir -p $(bin_path)/executables)
VPATH := $(VPATH) $(bin_path)/executables $(bin_path)/../../user/assembly/
