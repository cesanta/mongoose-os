REPO_PATH ?= ../../..
COMMON_PATH = $(REPO_PATH)/common
SRC_PATH ?= ../../src
V7_PATH ?= ../../../v7
MONGOOSE_PATH ?= ../../../mongoose
BIN_DIR ?= ./bin
BUILD_DIR ?= ./build
OUTAPP ?= smartjs
VERBOSE ?= 0
SSL ?= OpenSSL

# For FW_VERSION, COMMON_V7_FEATURES, MG_FEATURES_TINY
include $(REPO_PATH)/smartjs/common.mk

V7_FEATURES ?= $(COMMON_V7_FEATURES) \
              -DV7_BUILD_PROFILE=3 -DV7_ENABLE__Memory__stats \
              -DV7_ENABLE_COMPACTING_GC \
              -DV7_ENABLE_FILE -DV7_MAIN -DV7_ALLOW_ARGLESS_MAIN \
              -DV7_ENABLE_UBJSON -DV7_ENABLE_ENTITY_IDS

SJ_FEATURES = -DCS_ENABLE_UBJSON -DSJ_PROMPT_DISABLE_ECHO
MONGOOSE_FEATURES = \
  -DMG_USE_READ_WRITE -DMG_ENABLE_THREADS -DMG_ENABLE_THREADS \
  -DMG_ENABLE_HTTP_STREAMING_MULTIPART

INCLUDES = $(REPO_PATH) $(SRC_PATH) $(BUILD_DIR)
APP_SRCS := $(notdir $(wildcard *.c)) v7.c sj_v7_ext.c \
            mongoose.c sj_i2c_js.c sj_spi_js.c sj_mongoose.c \
            sj_mongoose_ws_client.c sj_mqtt.c sj_gpio_js.c sj_adc_js.c \
            sj_prompt.c sj_timers.c sj_timers_mongoose.c sj_uart.c sj_http.c \
            sj_debug_js.c sj_pwm_js.c sj_wifi_js.c clubby_proto.c \
            ubjserializer.c sj_clubby.c sj_common.c

# inline causes crashes in the compacting GC
# TODO(mkm) figure out which functions are inline sensitive and annotate them
CFLAGS_EXTRA =
CFLAGS ?= -O2 -fno-inline -W -Wall -Werror -g -Wno-unused-function \
          -D_DEFAULT_SOURCE \
          -D_GNU_SOURCE \
          $(V7_FEATURES) \
          $(SJ_FEATURES) \
          $(MONGOOSE_FEATURES) \
          -DFW_ARCHITECTURE=\"$(PLATFORM)\" \
          $(CFLAGS_EXTRA)

include $(COMMON_PATH)/scripts/platform.mk

ifeq ($(OS),Windows_NT)
  PLATFORM="WIN"
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    PLATFORM="LINUX"
  else
    PLATFORM="POSIX"
  endif
endif

ifeq "$(SSL)" "OpenSSL"
  MONGOOSE_FEATURES += -DMG_ENABLE_SSL
  ADD_LIBS += ssl crypto
  ifeq ($(PLATFORM), "WIN")
    ADD_LIBS += gdi32
  endif
endif

# Non Linux
ifneq ($(PLATFORM), "LINUX")
  SJ_FEATURES += -DSJ_DISABLE_I2C -DSJ_DISABLE_SPI -DSJ_DISABLE_GPIO
endif

# Non Windows
ifneq ($(PLATFORM), "WIN")
  ADD_LIBS += m pthread
  V7_FEATURES += -DCS_ENABLE_NATIVE_MD5  # Mongoose uses cs_md5
endif

# Linux
ifeq ($(PLATFORM), "LINUX")
  ADD_LIBS += rt
endif

# Windows
ifeq ($(PLATFORM), "WIN")
  CFLAGS += -D_WIN32_WINNT=0x0500
  ADD_LIBS += ws2_32
endif

ifeq ($(GC_CHECK),1)
CFLAGS += -fno-optimize-sibling-calls -fno-omit-frame-pointer -fno-inline -finstrument-functions -DV7_ENABLE_GC_CHECK
endif

ifeq ($(ASAN),1)
  CFLAGS += -fsanitize=address -fcolor-diagnostics -fno-common
  LDFLAGS += -fsanitize=address
  CC = $(CLANG)
endif

INCDIRS = $(addprefix -I,$(INCLUDES))
LIBS = $(addprefix -l,$(ADD_LIBS))

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
BUILD_INFO_C = $(BUILD_DIR)/build_info.c
OBJS = $(APP_OBJS) $(BUILD_DIR)/build_info.o
VPATH = $(SRC_PATH) $(V7_PATH) $(MONGOOSE_PATH) $(COMMON_PATH)
GENFILES_FLAG = $(BUILD_DIR)/genfiles.done
GENFILES_LIST = $(LD_SCRIPT) $(BUILD_DIR)/sys_config.c

FS_FILES = $(wildcard $(SRC_PATH)/js/* $(SRC_PATH)/fs/*)

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

.PHONY: all clean fs $(V7_PATH)/v7.c

define compile
@echo "  CC $< -> $@"
$(Q) $(CC) -MD $(INCDIRS) $(CFLAGS) $1 -c $< -o $@
endef

all: $(BIN_DIR) $(BUILD_DIR) $(BIN_DIR)/$(OUTAPP) fs

include $(REPO_PATH)/smartjs/src/sys_config.mk

$(BIN_DIR) $(BUILD_DIR):
	$(vecho) MKDIR $@
	$(Q) mkdir -p $@

# This rule is for normal (pre-existing) sources from VPATH.
$(BUILD_DIR)/%.o: %.c $(GENFILES_FLAG)
	$(compile)

# This one is for generated sources in build directory.
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c $(GENFILES_FLAG)
	$(compile)

$(BUILD_DIR)/mongoose.o: mongoose.c
	$(compile) -DEXCLUDE_COMMON

$(BUILD_DIR)/build_info.o: $(BUILD_INFO_C)
	$(compile)

# Common gathering point for all generated files.
# Except build info, which is special because it depends on objects.
$(GENFILES_FLAG): $(GENFILES_LIST)
	$(Q) touch $@

# Generate build info. Only regenerates if there are changes in objects.
$(BUILD_INFO_C): $(APP_OBJS)
	@echo "  GEN $@"
	$(Q) $(COMMON_PATH)/tools/fw_meta.py gen_build_info \
		--c_output=$(BUILD_INFO_C)

-include $(wildcard $(BUILD_DIR)/*.d)

$(V7_PATH)/v7.c:
	@make -C $(V7_PATH) v7.c

$(BIN_DIR)/$(OUTAPP): $(BIN_DIR) $(V7_PATH)/v7.c $(OBJS)
	@echo "LD $@"
	$(Q) $(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $@

# After file copy, change default HTTP port to 9080
fs: $(BIN_DIR) $(FS_FILES)
	@echo "MKFS $@"
	$(Q) cp $(FS_FILES) $(BIN_DIR)
	$(Q) perl -pi"" -e 's/"80"/"9080"/' $(BIN_DIR)/conf_sys_defaults.json

clean:
	$(Q) rm -rf $(BIN_DIR) $(BUILD_DIR)

