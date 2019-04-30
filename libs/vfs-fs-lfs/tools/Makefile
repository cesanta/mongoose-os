.DEFAULT_GOAL = all

FROZEN_PATH ?= $(CURDIR)/../../../frozen
LFS_SRC_PATH ?= $(CURDIR)/../src/littlefs
LFS_INCLUDE_PATH ?= $(CURDIR)/../src/littlefs

LFS_TOOLS_CFLAGS = -Wall -Werror -DNO_ESP -DLFS_TEST_VISUALISATION -DLFS_TOOLS_BUILD -g

all: mklfs

define compile
	$(Q) cc -I. -I$(LFS_INCLUDE_PATH) -I$(FROZEN_PATH) -o $@ $^ $(LFS_TOOLS_CFLAGS) $1
endef

COMMON_FILES = mem_lfs.c $(LFS_SRC_PATH)/lfs.c $(LFS_SRC_PATH)/lfs_util.c $(FROZEN_PATH)/frozen.c

mklfs: mklfs.c $(COMMON_FILES)
	@echo GCC mkspiffs
	$(call compile,)

clean:
	@echo CLEAN
	$(Q) rm -f mklfs
