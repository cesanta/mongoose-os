include common.mk

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

# espconn API is disabled, to enable add -DESP_ENABLE_ESPCONN below.
#
# Lowering TCP_MSL avoids long TIME_WAIT and LAST_ACK states.

CFLAGS += -w -DLWIP_OPEN_SRC -DPBUF_RSV_FOR_WLAN -DEBUF_LWIP \
          -DLWIP_RANDOMIZE_INITIAL_LOCAL_PORTS -DLWIP_RAND=os_random \
          -DTCP_MSL=3000 $(CFLAGS_EXTRA)

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
