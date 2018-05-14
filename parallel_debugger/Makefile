ROOT = ..
TARGETS = parallel_debugger

# Path to libelfin library
LIBELFIN_PATH="../../libelfin/"
export PKG_CONFIG_PATH=$(LIBELFIN_PATH)/elf:$(LIBELFIN_PATH)/dwarf

CXXFLAGS += --std=c++11 -I$(LIBELFIN_PATH)/elf -I$(LIBELFIN_PATH)/dwarf
LDFLAGS = -L$(LIBELFIN_PATH)/elf -L$(LIBELFIN_PATH)/dwarf -Wl,-R$(LIBELFIN_PATH)/elf,-R$(LIBELFIN_PATH)/dwarf

LIBS = dwarf++ elf++

include $(ROOT)/common.mk
