V7_PATH ?= ../../../v7
FOSSA_PATH = ?= ../../../fossa
SRC_PATH ?= ../../src
COMMON_PATH ?= ../../../common
CLUBBY_PATH ?= $(SRC_PATH)
BIN_DIR ?= ./bin
BUILD_DIR ?= ./build
OUTAPP ?= smartjs
VERBOSE ?= 0
SSL ?= OpenSSL

V7_FEATURES = -DV7_BUILD_PROFILE=3 -DV7_ENABLE__Memory__stats \
								   -DV7_ENABLE_COMPACTING_GC -DDISABLE_MD5 \
								   -DV7_ENABLE_FILE -DV7_MAIN -DV7_ALLOW_ARGLESS_MAIN \
								   -DNS_ENABLE_THREADS -DSJ_DISABLE_ECHO -DV7_ENABLE__StackTrace
SJ_FEATURES =
FOSSA_FEATURES =

INCLUDES = $(V7_PATH) $(SRC_PATH) $(COMMON_PATH) $(FOSSA_PATH)
APP_SRCS := $(notdir $(wildcard *.c)) v7.c sj_v7_ext.c sj_conf.c \
			sj_simple_http.c fossa.c sj_i2c_js.c sj_spi_js.c sj_fossa.c \
			sj_fossa_http_client.c sj_fossa_ws_client.c sj_gpio_js.c \
			sj_prompt.c
ADD_LIBS = m pthread

ifeq ($(OS),Windows_NT)
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    ADD_LIBS += rt
  endif
  ifneq ($(UNAME_S),Linux)
		SJ_FEATURES += -DSJ_DISABLE_I2C -DSJ_DISABLE_SPI -DSJ_DISABLE_GPIO
	endif
  ifeq "$(SSL)" "OpenSSL"
    FOSSA_FEATURES += -DNS_ENABLE_SSL
    ADD_LIBS += ssl crypto
  endif
endif

ifeq ($(VERBOSE),1)
Q :=
else
Q := @
endif

# inline causes crashes in the compacting GC
# TODO(mkm) figure out which functions are inline sensitive and annotate them
CFLAGS_EXTRA =
CFLAGS ?= -O2 -fno-inline -Wall -Werror -g -Wno-pragmas -D_BSD_SOURCE -D_GNU_SOURCE $(V7_FEATURES) $(SJ_FEATURES) $(FOSSA_FEATURES) -Wno-unused-function $(CFLAGS_EXTRA)

ifeq ($(GC_CHECK),1)
CFLAGS += -fno-optimize-sibling-calls -fno-omit-frame-pointer -fno-inline -finstrument-functions -DV7_ENABLE_GC_CHECK
endif

INCDIRS = $(addprefix -I,$(INCLUDES))
LIBS = $(addprefix -l,$(ADD_LIBS))

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
OBJS = $(APP_OBJS) $(BUILD_DIR)/sj_version.o
VPATH = $(V7_PATH) $(SRC_PATH) $(COMMON_PATH) $(FOSSA_PATH)

FS = $(wildcard $(SRC_PATH)/js/*.js) $(CLUBBY_PATH)/clubby.js

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

$(BUILD_DIR)/fossa.o: fossa.c
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

$(BIN_DIR)/$(OUTAPP): $(OBJS)
	@echo "LD $@"
	$(Q) $(CC) $(OBJS) $(LIBS) -o $@

fs: $(FS)
	@echo "MKFS $@"
	$(Q) cp $(FS) $(BIN_DIR)

clean:
	$(Q) rm -rf $(BIN_DIR)/$(OUTAPP) $(BUILD_DIR)
