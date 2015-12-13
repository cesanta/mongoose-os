SRC_PATH ?= ../../src
V7_PATH ?= ../../../v7
BIN_DIR ?= ./bin
BUILD_DIR ?= ./build
OUTAPP ?= smartjs
VERBOSE ?= 0
SSL ?= OpenSSL

V7_FEATURES = -DV7_BUILD_PROFILE=3 -DV7_ENABLE__Memory__stats \
			  -DV7_ENABLE_COMPACTING_GC \
			  -DV7_ENABLE_FILE -DV7_MAIN -DV7_ALLOW_ARGLESS_MAIN \
			  -DMG_ENABLE_THREADS -DSJ_PROMPT_DISABLE_ECHO \
			  -DCS_ENABLE_UBJSON -DV7_ENABLE_UBJSON
SJ_FEATURES =
MONGOOSE_FEATURES = -DMG_USE_READ_WRITE

INCLUDES = $(SRC_PATH)
APP_SRCS := $(notdir $(wildcard *.c)) v7.c sj_v7_ext.c \
			mongoose.c sj_i2c_js.c sj_spi_js.c sj_mongoose.c \
			sj_mongoose_ws_client.c sj_gpio_js.c \
			sj_prompt.c sj_timers.c sj_uart.c sj_http.c

# inline causes crashes in the compacting GC
# TODO(mkm) figure out which functions are inline sensitive and annotate them
CFLAGS_EXTRA =
CFLAGS ?= -O2 -fno-inline -Wall -Werror -g -D_BSD_SOURCE -D_GNU_SOURCE $(V7_FEATURES) $(SJ_FEATURES) $(MONGOOSE_FEATURES) -Wno-unused-function $(CFLAGS_EXTRA)

include $(SRC_PATH)/platform.mk

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

ifeq ($(VERBOSE),1)
Q :=
else
Q := @
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
OBJS = $(APP_OBJS) $(BUILD_DIR)/sj_version.o
VPATH = $(SRC_PATH)

FS = $(wildcard $(SRC_PATH)/js/*.js)

.PHONY: all clean

define compile
@echo "CC $< -> $@"
$(Q) $(CC) -MD $(INCDIRS) $(CFLAGS) $1 -c $< -o $@
endef

all: $(BIN_DIR) $(BUILD_DIR) $(BIN_DIR)/$(OUTAPP) fs

$(BIN_DIR):
	$(Q) mkdir -p $@

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(BUILD_DIR)/%.o: %.c
	$(compile)

$(BUILD_DIR)/mongoose.o: mongoose.c
	$(compile) -DEXCLUDE_COMMON

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	$(compile)

$(BUILD_DIR)/sj_version.c: $(APP_SRCS)
	@echo "GEN sj_version.c"
	@REF=$$(git symbolic-ref HEAD | sed 's:refs/heads/::'); \
	SHA=$$(git rev-parse HEAD | head -c 8); \
	DATE=$$(TZ=GMT date +"%Y%m%d-%H%M%S"); \
	echo "const char *sj_version = \"POSIX-$$REF/$$DATE/$$SHA\";" > $(BUILD_DIR)/sj_version.c

-include $(wildcard $(BUILD_DIR)/*.d)

$(SRC_PATH)/v7.c:
	@make -C $(V7_PATH) v7.c
	@ln -s ../../v7/v7.c $(SRC_PATH)/v7.c

$(BIN_DIR)/$(OUTAPP): $(OBJS)
	@echo "LD $@"
	$(Q) $(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $@

fs: $(FS)
	@echo "MKFS $@"
	$(Q) cp $(FS) $(BIN_DIR)

clean:
	$(Q) rm -rf $(BIN_DIR)/$(OUTAPP) $(BUILD_DIR)

