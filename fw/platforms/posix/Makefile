# -*- mode: makefile -*-
#
# Mongoose OS for POSIX

MAKEFLAGS += --warn-undefined-variables
.DEFAULT_GOAL := all

MGOS_PATH ?= ../../..
COMMON_PATH ?= $(MGOS_PATH)/common
MGOS_SRC_PATH ?= $(MGOS_PATH)/fw/src
SRC_PATH ?= ../../src
MONGOOSE_PATH ?= ../../../mongoose
BIN_DIR ?= ./bin
BUILD_DIR ?= ./.build
GEN_DIR ?= ./.build/gen
FROZEN_PATH ?= $(MGOS_PATH)/frozen
APP ?= mongoose-os
VERBOSE ?= 0
VPATH = $(SRC_PATH) $(MONGOOSE_PATH) $(FROZEN_PATH) $(COMMON_PATH) $(COMMON_PATH)/mg_rpc
SSL ?= None
DEBUG ?= 0
CC_WRAPPER ?=

MGOS_ENABLE_ATCA = 0
MGOS_ENABLE_ATCA_SERVICE = 0

# Explicitly disable updater, it's not supported on POSIX build yet.
MGOS_ENABLE_UPDATER = 0
MGOS_ENABLE_UPDATER_POST = 0
MGOS_ENABLE_UPDATER_RPC = 0
MGOS_ENABLE_DNS_SD = 0  # Not supported on POSIX
MGOS_ENABLE_I2C ?= 1
MGOS_ENABLE_I2C_GPIO = 0
MGOS_ENABLE_SYS_SERVICE = 0
MGOS_ENABLE_WIFI = 0

# For FW_VERSION, MG_FEATURES_TINY
include $(MGOS_PATH)/fw/common.mk

LDFLAGS ?=

MGOS_POSIX_FEATURES ?= -DMGOS_PROMPT_DISABLE_ECHO -DMGOS_MAX_NUM_UARTS=2 -DMGOS_NUM_GPIO=16
MONGOOSE_FEATURES = \
  -DMG_USE_READ_WRITE -DMG_ENABLE_THREADS -DMG_ENABLE_THREADS \
  -DMG_ENABLE_HTTP_STREAMING_MULTIPART -DMG_DISABLE_DAV


INCLUDES = $(MGOS_PATH) $(SRC_PATH) $(BUILD_DIR)
MGOS_SRCS = $(notdir $(wildcard *.c)) mgos_init.c  \
            mongoose.c frozen.c mgos_mongoose.c mgos_gpio.c \
            mgos_timers_mongoose.c \
            mgos_config.c mgos_sys_config.c \
            $(notdir $(SYS_CONFIG_C)) $(notdir $(SYS_RO_VARS_C)) \
            json_utils.c cs_rbuf.c mgos_uart.c \
            mgos_utils.c cs_file.c

include $(MGOS_PATH)/fw/src/features.mk

# inline causes crashes in the compacting GC
# TODO(mkm) figure out which functions are inline sensitive and annotate them
CFLAGS_EXTRA =
CFLAGS ?= -std=c99 -fno-inline -W -Wall -Wundef -Werror -g -Wno-unused-function \
          -Wno-missing-field-initializers \
          -D_DEFAULT_SOURCE \
          -D_GNU_SOURCE \
          $(MGOS_FEATURES) $(MGOS_POSIX_FEATURES) \
          $(MONGOOSE_FEATURES) \
          -DMGOS_APP=\"$(APP)\" -DFW_ARCHITECTURE=$(BUILD_PLATFORM) \
          $(CFLAGS_EXTRA)

ifeq "$(DEBUG)" "1"
CFLAGS += -O0
else
CFLAGS += -O2
endif

include $(COMMON_PATH)/scripts/platform.mk

ifeq ($(OS),Windows_NT)
  BUILD_PLATFORM="WIN"
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    BUILD_PLATFORM="LINUX"
    ifeq "$(MGOS_ENABLE_I2C)" "1"
      SYS_CONF_SCHEMA += lnx_i2c_config.yaml
    endif
  else
    BUILD_PLATFORM="POSIX"
  endif
endif

ifeq "$(SSL)" "OpenSSL"
  MONGOOSE_FEATURES += -DMG_ENABLE_SSL
  ADD_LIBS += ssl crypto
  ifeq ($(BUILD_PLATFORM), "WIN")
    ADD_LIBS += gdi32
  endif
endif

# Non Windows
ifneq ($(BUILD_PLATFORM), "WIN")
  ADD_LIBS += m pthread
endif

# Linux
ifeq ($(BUILD_PLATFORM), "LINUX")
  ADD_LIBS += rt
endif

# Windows
ifeq ($(BUILD_PLATFORM), "WIN")
  CFLAGS += -D_WIN32_WINNT=0x0500
  ADD_LIBS += ws2_32
endif

ifeq "$(SSL)" "OpenSSL"
CFLAGS += -DMG_ENABLE_SSL
LDFLAGS += -lssl -lcrypto
endif

GC_CHECK ?=
ifeq ($(GC_CHECK),1)
CFLAGS += -fno-optimize-sibling-calls -fno-omit-frame-pointer -fno-inline -finstrument-functions
endif

ASAN ?=
ifeq ($(ASAN),1)
  CFLAGS += -fsanitize=address -fcolor-diagnostics -fno-common
  LDFLAGS += -fsanitize=address
  CC = clang-3.6
endif

INCDIRS = $(addprefix -I,$(INCLUDES))
LIBS = $(addprefix -l,$(ADD_LIBS))

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MGOS_SRCS))
SYS_CONFIG_C = $(BUILD_DIR)/sys_config.c
SYS_CONFIG_DEFAULTS_JSON = $(BUILD_DIR)/sys_config_defaults.json
SYS_RO_VARS_C = $(BUILD_DIR)/sys_ro_vars.c
BUILD_INFO_C = $(BUILD_DIR)/build_info.c
OBJS = $(APP_OBJS) $(BUILD_DIR)/build_info.o
GENFILES_FLAG = $(BUILD_DIR)/genfiles.done
GENFILES_LIST = $(SYS_CONFIG_C) $(SYS_RO_VARS_C)

FS_FILES = $(wildcard $(SRC_PATH)/js/* $(MGOS_PATH)/fw/skeleton/fs/*)

# Custom conf.json : by default, it is empty, so that the one from `fs` is
# used. NOTE: if the custom one is provided, the file should anyway be named
# `conf.json`
CONF_JSON ?= ""

# If custom conf.json was provided, use it
ifneq ($(CONF_JSON),"")
  # Note: instead of substituting one with another one, we separately remove
  # the old one, and add a new one, to make it work even if `fs` does not
  # contain `conf.json`.
  FS_FILES := $(subst $(SRC_PATH)/fs/conf.json,,$(FS_FILES))
  FS_FILES += $(CONF_JSON)
endif

.PHONY: all clean fs

define compile
$(vecho) "CC    $< -> $@"
$(Q) $(CC_WRAPPER) $(CC) -MD $(INCDIRS) $(CFLAGS) $1 -c $< -o $@
endef

all: $(BIN_DIR) $(BUILD_DIR) $(BIN_DIR)/$(APP) fs

include $(MGOS_PATH)/fw/src/sys_config.mk

$(BIN_DIR) $(BUILD_DIR):
	$(vecho) MKDIR $@
	$(Q) mkdir -p $@

# This rule is for normal (pre-existing) sources from VPATH.
$(BUILD_DIR)/%.o: %.c $(GENFILES_FLAG)
	$(call compile,)

# This one is for generated sources in build directory.
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c $(GENFILES_FLAG)
	$(call compile,)

$(BUILD_DIR)/build_info.o: $(BUILD_INFO_C)
	$(call compile,)

# Common gathering point for all generated files.
# Except build info, which is special because it depends on objects.
$(GENFILES_FLAG): $(GENFILES_LIST)
	$(Q) touch $@

# Generate build info. Only regenerates if there are changes in objects.
include $(MGOS_PATH)/common/scripts/build_info.mk

$(BUILD_INFO_C): $(APP_OBJS)
	$(call gen_build_info,$@,,,,,$(BUILD_INFO_C),)

-include $(wildcard $(BUILD_DIR)/*.d)

$(BIN_DIR)/$(APP): $(BIN_DIR) $(OBJS)
	@echo "LD $@"
	$(Q) $(CC_WRAPPER) $(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $@

# After file copy, change default HTTP port to 9080
fs: $(BIN_DIR) $(FS_FILES) $(SYS_CONFIG_C)
	@echo "MKFS $@"
	$(Q) cp $(FS_FILES) $(BIN_DIR)
	$(Q) cp $(SYS_CONFIG_DEFAULTS_JSON) $(BIN_DIR)/conf_defaults.json
	$(Q) perl -pi"" -e 's/"80"/"9080"/' $(BIN_DIR)/conf_defaults.json

clean:
	$(Q) rm -rf $(BIN_DIR) $(BUILD_DIR)
