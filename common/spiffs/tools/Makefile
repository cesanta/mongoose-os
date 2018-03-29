.DEFAULT_GOAL = all

BUILD_DIR ?= .
SPIFFS_PATH ?= $(CURDIR)/..
SPIFFS_CONFIG_PATH ?=
COMMON_PATH ?= $(CURDIR)/../..

ifeq "$(SPIFFS_CONFIG_PATH)" ""
$(error Please set SPIFFS_CONFIG_PATH to where spiffs_config.h for the platform you are building for is located)
endif

SPIFFS_TOOLS_CFLAGS = -Wall -Werror -DNO_ESP -DSPIFFS_TEST_VISUALISATION -DSPIFFS_TOOLS_BUILD -g

all: $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/mkspiffs8 $(BUILD_DIR)/unspiffs $(BUILD_DIR)/unspiffs8

define compile
	$(Q) gcc -I. -I$(COMMON_PATH)/.. -I$(SPIFFS_CONFIG_PATH) -I$(SPIFFS_PATH) -o $@ $^ $(SPIFFS_TOOLS_CFLAGS) $1
endef

COMMON_FILES = $(COMMON_PATH)/cs_crc32.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)

$(BUILD_DIR)/spls: spls.c $(COMMON_FILES)
	$(call compile,)

$(BUILD_DIR)/spls8: spls.c $(COMMON_FILES)
	$(call compile,-DCS_SPIFFS_ENABLE_METADATA)

$(BUILD_DIR)/mkspiffs: mkspiffs.c $(COMMON_FILES)
	@echo GCC mkspiffs
	$(call compile,)

$(BUILD_DIR)/mkspiffs8: mkspiffs.c $(COMMON_FILES)
	@echo GCC mkspiffs8
	$(call compile,-DCS_SPIFFS_ENABLE_METADATA)

$(BUILD_DIR)/unspiffs: unspiffs.c $(COMMON_FILES)
	@echo GCC unspiffs
	$(call compile,)

$(BUILD_DIR)/unspiffs8: unspiffs.c $(COMMON_FILES)
	@echo GCC unspiffs8
	$(call compile,-DCS_SPIFFS_ENABLE_METADATA)

clean:
	@echo CLEAN
	$(Q) rm -f $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/unspiffs $(BUILD_DIR)/sptest $(BUILD_DIR)/spls
