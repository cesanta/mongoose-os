include ./../common.mk


BUILD_DIR ?= ../build/lwip
LWIP_PATH = .
APP = liblwip_cs

APP_MODULES := $(LWIP_PATH)/src/netif $(LWIP_PATH)/src/core \
							 $(LWIP_PATH)/src/core/ipv4 $(LWIP_PATH)/src/api \
							 $(LWIP_PATH)/espressif/app

APP_SRCS := $(notdir $(foreach m,$(APP_MODULES),$(wildcard $(m)/*.c)))
VPATH = $(APP_MODULES)
IPATH = $(VPATH) $(LWIP_PATH)/src/include $(LWIP_PATH)/espressif/include \
				$(LWIP_PATH)/src/include/ipv4

.PHONY: all clean

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
SRCS = $(APP_SRCS)
OBJS = $(APP_OBJS)
APP_AR = $(BUILD_DIR)/$(APP).a
INCDIRS = $(addprefix -I,$(IPATH))

CFLAGS += -w -DICACHE_FLASH -DLWIP_OPEN_SRC -DPBUF_RSV_FOR_WLAN -DEBUF_LWIP \
          -DLWIP_RANDOMIZE_INITIAL_LOCAL_PORTS -DLWIP_RAND=os_random

all: $(BUILD_DIR) $(APP_AR)

$(BUILD_DIR):
	$(Q) mkdir -p $@

# Main app lib.
$(APP_AR): $(OBJS)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^
	$(Q) $(OBJCOPY) --rename-section .text=.irom0.text $(APP_AR)

# This rule is for normal (pre-existing) sources from VPATH.
$(BUILD_DIR)/%.o: %.c
	$(compile)

clean:
	$(Q) rm -rf $(BUILD_DIR)
