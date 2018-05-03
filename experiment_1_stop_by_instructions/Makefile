ROOT = ..
TARGETS = experiment

# Find libs
LIBELFIN_PATH="../../libelfin/"
export PKG_CONFIG_PATH=$(LIBELFIN_PATH)/elf:$(LIBELFIN_PATH)/dwarf
CXXFLAGS+=--std=c++11 -I$(LIBELFIN_PATH)/elf -I$(LIBELFIN_PATH)/dwarf
# Statically link against our libs to keep the example binaries simple
# and dependencies correct.
# LIBS=$(LIBELFIN_PATH)/dwarf/libdwarf++.a $(LIBELFIN_PATH)/elf/libelf++.a
LDFLAGS = -L$(LIBELFIN_PATH)/elf -L$(LIBELFIN_PATH)/dwarf -Wl,-R$(LIBELFIN_PATH)/elf,-R$(LIBELFIN_PATH)/dwarf
LIBS = dwarf++ elf++

include $(ROOT)/common.mk
