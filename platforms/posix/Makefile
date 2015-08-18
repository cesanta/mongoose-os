V7_PATH ?= ../../../v7
FOSSA_PATH = ?= ../../../fossa
SRC_PATH ?= ../../src
COMMON_PATH ?= ../../../common
BIN_DIR ?= ./bin
BUILD_DIR ?= ./build
OUTAPP ?= smartjs
VERBOSE ?= 0
SSL ?= OpenSSL

V7_FEATURES = -DV7_BUILD_PROFILE=3 -DV7_ENABLE__Memory__stats \
								   -DV7_ENABLE_GC -DV7_ENABLE_GC \
								   -DV7_ENABLE_COMPACTING_GC -DDISABLE_MD5 \
								   -DV7_ENABLE_FILE
SJ_FEATURES =
FOSSA_FEATURES =

INCLUDES = $(V7_PATH) $(SRC_PATH) $(COMMON_PATH) $(FOSSA_PATH)
APP_SRCS := $(notdir $(wildcard *.c)) v7.c sj_v7_ext.c sj_conf.c \
			sj_simple_http.c fossa.c sj_i2c_js.c sj_spi_js.c sj_fossa.c \
			sj_fossa_http_client.c sj_fossa_ws_client.c
ADD_LIBS = m

ifeq ($(OS),Windows_NT)
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    ADD_LIBS += rt
  endif
  ifneq ($(UNAME_S),Linux)
		SJ_FEATURES += -DSJ_DISABLE_I2C -DSJ_DISABLE_SPI
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

CFLAGS ?= -O2 -Wall -g -Wno-pragmas -D_BSD_SOURCE $(V7_FEATURES) $(SJ_FEATURES) $(FOSSA_FEATURES) -Wno-unused-function

INCDIRS = $(addprefix -I,$(INCLUDES))
LIBS = $(addprefix -l,$(ADD_LIBS))

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
OBJS = $(APP_OBJS) $(BUILD_DIR)/sj_version.o
VPATH = $(V7_PATH) $(SRC_PATH) $(COMMON_PATH) $(FOSSA_PATH)

.PHONY: all clean

define compile
@echo "CC $< -> $@"
$(Q) $(CC) -MD $(INCDIRS) $(CFLAGS) $1 -c $< -o $@
endef

all: $(BIN_DIR) $(BUILD_DIR) $(BIN_DIR)/$(OUTAPP)

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

clean:
	$(Q) rm -rf $(BIN_DIR)/$(OUTAPP) $(BUILD_DIR)
