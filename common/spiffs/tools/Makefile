.DEFAULT_GOAL = all

include ../../../fw/common.mk

BUILD_DIR ?= .
SPIFFS_PATH ?= $(CURDIR)/..
SPIFFS_CONFIG_PATH ?=

ifeq "$(SPIFFS_CONFIG_PATH)" ""
$(error Please set SPIFFS_CONFIG_PATH to where spiffs_config.h for the platform you are building for is located)
endif

SPIFFS_TOOLS_CFLAGS = -Wall -Werror -DNO_ESP -DSPIFFS_TEST_VISUALISATION -g

all: $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/mkspiffs8 $(BUILD_DIR)/unspiffs $(BUILD_DIR)/unspiffs8

define compile
	$(Q) gcc -I. -I$(SPIFFS_CONFIG_PATH) -I$(SPIFFS_PATH) -o $@ $^ $(SPIFFS_TOOLS_CFLAGS) $1
endef

$(BUILD_DIR)/mkspiffs: mkspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC mkspiffs"
	$(call compile,)

$(BUILD_DIR)/mkspiffs8: mkspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC mkspiffs8"
	$(call compile,-DSPIFFS_OBJ_META_LEN=8)

$(BUILD_DIR)/unspiffs: unspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC unspiffs"
	$(call compile,)

$(BUILD_DIR)/unspiffs8: unspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC unspiffs8"
	$(call compile,-DSPIFFS_OBJ_META_LEN=8)

clean:
	$(vecho) "CLEAN"
	$(Q) rm -f $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/unspiffs
