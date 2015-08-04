V7_PATH ?= ../../../v7
SRC_PATH ?= ../../src
COMMON_PATH ?= ../../../common
BIN_DIR ?= ./bin
BUILD_DIR ?= ./build
OUTAPP ?= smartjs
VERBOSE ?= 0

CC = gcc
LD = gcc
V7_FEATURES = -DV7_BUILD_PROFILE=3 -DV7_ENABLE__Memory__stats

INCLUDES = $(V7_PATH) $(SRC_PATH) $(COMMON_PATH)
APP_SRCS := $(notdir $(wildcard *.c)) v7.c sj_v7_ext.c sj_conf.c

CFLAGS ?= -O2 -Wall -D_BSD_SOURCE $(V7_FEATURES) -Wno-unused-function
ADD_LIBS = m rt

ifeq ($(VERBOSE),1)
Q :=
else
Q := @
endif

INCDIRS = $(addprefix -I,$(INCLUDES))
LIBS = $(addprefix -l,$(ADD_LIBS))

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
OBJS = $(APP_OBJS) $(BUILD_DIR)/sj_version.o
VPATH = $(V7_PATH) $(SRC_PATH) $(COMMON_PATH)

.PHONY: all clean

define compile
@echo "CC $< -> $@"
$(Q) $(CC) -MD $(INCDIRS) $(CFLAGS) -c $< -o $@
endef

all:  $(BIN_DIR) $(BUILD_DIR) $(BIN_DIR)/$(OUTAPP)

$(BIN_DIR):
	$(Q) mkdir -p $@

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(BUILD_DIR)/%.o: %.c
	@$(compile)

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	@$(compile)

$(BUILD_DIR)/sj_version.c: $(APP_SRCS)
	@echo "GEN sj_version.c"
	@REF=$$(git symbolic-ref HEAD | sed 's:refs/heads/::'); \
	SHA=$$(git rev-parse HEAD | head -c 8); \
	DATE=$$(TZ=GMT date +"%Y%m%d-%H%M%S"); \
	echo "const char *sj_version = \"POSIX-$$REF/$$DATE/$$SHA\";" > $(BUILD_DIR)/sj_version.c

-include $(wildcard $(BUILD_DIR)/*.d)

$(BIN_DIR)/$(OUTAPP): $(OBJS)
	@echo "LD $@"
	$(Q) $(LD) -Wl,--start-group $(LIBS) $(OBJS) -Wl,--end-group -o $@

clean:
	$(Q) rm -rf $(BIN_DIR)/$(OUTAPP) $(BUILD_DIR)
