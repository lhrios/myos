# makefile @ kernel folder

CC_FLAGS += -DKERNEL_CODE
SHELL = /bin/bash

dep_path := ../$(dep_path)/kernel
bin_path_base := ../$(bin_path_base)
bin_path := ../$(bin_path)/kernel
executables += $(bin_path)/kernel
src_path := ../$(src_path)/

include ../general_makefile
include ../c_general_makefile

dependency_objects_with_path += $(foreach file, \
	$(shell find ../common/standard_library -iname '*.c'), \
	$(bin_path)/$(patsubst %.c,%.o,$(file)))
dependency_objects_with_path += $(foreach file, \
	$(shell find ../common/util -iname '*.c'), \
	$(bin_path)/$(patsubst %.c,%.o,$(file)))
dependency_objects_with_path += $(foreach file, \
	$(shell find assembly -iname '*.asm'), \
	$(bin_path)/$(patsubst %.asm,%_asm.o,$(file)))

filtered_dependency_objects_with_path := $(dependency_objects_with_path)
filtered_dependency_objects = $(foreach path, $(filtered_dependency_objects_with_path), $(notdir $(path)))

$(bin_path)/kernel : $(objects) $(filtered_dependency_objects) makefile
	@echo creating $(@F)
	@$(LD) $(LD_FLAGS) $(objects_with_path) $(filtered_dependency_objects_with_path) -o $@

VPATH := $(VPATH) $(bin_path)/assembly \
	$(bin_path)/file_system \
	$(bin_path)/services \
	$(bin_path)/io \
	file_system \
	services \
	io \
	$(bin_path)/../common/standard_library \
	$(bin_path)/../common/util
