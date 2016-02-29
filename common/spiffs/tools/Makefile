include ../../../smartjs/common.mk

BUILD_DIR ?= .
SPIFFS_PATH ?= $(CURDIR)/..

$(BUILD_DIR)/mkspiffs: mkspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC mkspiffs"
	$(Q) gcc -I. -I$(SPIFFS_PATH) -I$(SPIFFS_PATH)/tools -Iuser -o $@ $^ -DNO_ESP

$(BUILD_DIR)/unspiffs: unspiffs.c mem_spiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC unspiffs"
	$(Q) gcc -I. -I$(SPIFFS_PATH) -I$(SPIFFS_PATH)/tools -Iuser -o $@ $^ -DNO_ESP

clean:
	$(vecho) "CLEAN"
	$(Q) rm -f $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/unspiffs
