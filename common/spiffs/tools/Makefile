include ../../../fw/common.mk

BUILD_DIR ?= .
SPIFFS_PATH ?= $(CURDIR)/..

SPIFFS_TOOLS_CFLAGS = -Wall -Werror -DNO_ESP -DSPIFFS_TEST_VISUALISATION -g

all: $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/unspiffs

$(BUILD_DIR)/mkspiffs: mkspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC mkspiffs"
	$(Q) gcc -I. -I$(SPIFFS_PATH) -I$(SPIFFS_PATH)/tools -Iuser -o $@ $^ $(SPIFFS_TOOLS_CFLAGS)

$(BUILD_DIR)/unspiffs: unspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC unspiffs"
	$(Q) gcc -I. -I$(SPIFFS_PATH) -I$(SPIFFS_PATH)/tools -Iuser -o $@ $^ $(SPIFFS_TOOLS_CFLAGS)

clean:
	$(vecho) "CLEAN"
	$(Q) rm -f $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/unspiffs
