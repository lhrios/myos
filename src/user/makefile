# makefile @ user folder

dep_path := ../$(dep_path)/user
bin_path_base := ../$(bin_path_base)
bin_path := ../$(bin_path)/user/
standard_library := $(bin_path)/standard_library/standard_library.a

executables += $(foreach file, \
	$(shell ls *.c), \
	$(patsubst %.c,%,$(file)))

$(shell test -d ../../../ncurses)
ifeq ($(.SHELLSTATUS), 1)
	executables := $(filter-out test_ncurses_special_characters,$(executables))
	sources_to_remove = ./test_ncurses_special_characters.c
endif

include ../general_makefile
include ../c_general_makefile

CC_FLAGS += -fno-zero-initialized-in-bss -I ../../../ncurses/include

executable_common_dependency_objects_with_path := $(bin_path)/assembly/executable_entry_point_asm.o
executable_common_dependency_objects = $(foreach path, $(executable_common_dependency_objects_with_path), $(notdir $(path)))

standard_library_dependency_objects_with_path += $(bin_path)/assembly/signal_handler_asm.o
standard_library_dependency_objects_with_path += $(bin_path)/assembly/setjmp_asm.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/math_utils.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/debug_utils.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/formatter.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/scanner.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/string_utils.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/double_linked_list.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/stream_reader.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/stream_writer.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/string_stream_reader.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/string_stream_writer.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/path_utils.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/command_line_utils.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/priority_queue.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/search_utils.o
standard_library_dependency_objects_with_path += $(bin_path_base)/common/util/date_time_utils.o
standard_library_dependency_objects_with_path += $(foreach file, \
	$(shell find standard_library myos_standard_library util -iname '*.c'), \
	$(bin_path)/$(patsubst %.c,%.o,$(file)))
standard_library_dependency_objects_with_path += $(foreach file, \
	$(shell find ../common/standard_library -iname '*.c'), \
	$(bin_path)/$(patsubst %.c,%.o,$(file)))

standard_library_dependency_objects = $(foreach path, $(standard_library_dependency_objects_with_path), $(notdir $(path)))

export test_ncurses_special_characters_dependencies_with_path += ../../../ncurses/lib/libncurses.a

export system_call_tester_dependencies_with_path += $(bin_path)/util/tty_utils.o

export test_tty_special_keys_dependencies_with_path += $(bin_path)/util/tty_utils.o

$(standard_library) : $(standard_library_dependency_objects) makefile
	@echo creating $(@F)
	@ar -c -r $(@) $(standard_library_dependency_objects_with_path)

$(executables) : % : %.o $(standard_library) $(executable_common_dependency_objects) makefile
	@echo creating $(@F)
	@ld -m elf_i386 -T script.ld --orphan-handling=discard \
		$(executable_common_dependency_objects_with_path) \
		$(bin_path)/$(<F) \
		$${$(@F)_dependencies_with_path} \
		$(standard_library) \
		--oformat=binary -o $(bin_path)/executables/$(@F)

VPATH := $(VPATH) $(bin_path)/executables \
	$(bin_path)/assembly \
	$(bin_path)/myos_standard_library \
	myos_standard_library \
	$(bin_path)/standard_library \
	standard_library \
	$(bin_path)/util \
	util \
	$(bin_path)/../common/standard_library \
	$(bin_path)/../common/util
