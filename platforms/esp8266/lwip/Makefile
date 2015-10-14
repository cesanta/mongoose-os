include ./../common.mk


BUILD_DIR = ../build/lwip
LWIP_PATH = .
APP = liblwip_cs

APP_MODULES := $(LWIP_PATH)/lwip/netif $(LWIP_PATH)/lwip/core/ $(LWIP_PATH)/lwip/core/ipv4 \
							$(LWIP_PATH)/lwip/app $(LWIP_PATH)/lwip/api

APP_SRCS := $(notdir $(foreach m,$(APP_MODULES),$(wildcard $(m)/*.c)))
VPATH = $(APP_MODULES)
IPATH = $(VPATH) $(LWIP_PATH)/include

.PHONY: all clean

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
SRCS = $(APP_SRCS)
OBJS = $(APP_OBJS)
APP_AR = $(BUILD_DIR)/$(APP).a
INCDIRS = $(addprefix -I,$(IPATH))

CFLAGS += -w -DICACHE_FLASH -DLWIP_OPEN_SRC -DPBUF_RSV_FOR_WLAN -DEBUF_LWIP

all: $(BUILD_DIR) $(APP_AR)

$(BUILD_DIR):
	$(Q) mkdir -p $@

# Main app lib.
$(APP_AR): $(OBJS)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

# This rule is for normal (pre-existing) sources from VPATH.
$(BUILD_DIR)/%.o: %.c
	$(compile)

clean:
	$(Q) rm -rf $(BUILD_DIR)
