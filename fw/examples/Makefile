PLATFORM ?= esp8266

REPO_ROOT = $(abspath ../..)
MJS_PATH = $(REPO_ROOT)/mjs
LIBS_PATH = $(REPO_ROOT)/mos_libs

EXAMPLES_cc3200 = c_aws_shadow c_hello c_http c_network c_mqtt c_uart
EXAMPLES_esp32 = c_arduino_bme280 c_arduino_dallas_temp c_arduino_dht c_arduino_onewire c_arduino_ssd1306 c_aws_shadow c_hello c_hello_arduino c_http c_mqtt c_rpc c_uart c_websocket mjs_arduino_onewire
EXAMPLES_esp8266 = c_arduino_bme280 c_arduino_dallas_temp c_arduino_dht c_arduino_onewire c_arduino_ssd1306 c_arduino_wire c_aws_shadow c_heater c_hello c_hello_arduino c_http c_mcast c_mqtt c_network c_uart c_websocket mjs_arduino_bme280 mjs_arduino_dallas_temp mjs_arduino_dht mjs_arduino_onewire
EXAMPLES_pic32 =
EXAMPLES_stm32 =

MOS ?= mos

$(eval EXAMPLES=$(EXAMPLES_$(PLATFORM)))

.PHONY: all $(EXAMPLES) list_examples

export PLATFORM

all: $(EXAMPLES)

$(EXAMPLES): $(MJS_PATH)/mjs.c $(MJS_PATH)/mjs.h
	@echo "=== Building $@ / $(PLATFORM)"
	@cd $@ && $(MOS) build --local --clean --arch $(PLATFORM) \
		--repo $(REPO_ROOT) \
		--lib aws:$(LIBS_PATH)/aws \
		--lib mjs:$(LIBS_PATH)/mjs \
		--lib blynk:$(LIBS_PATH)/blynk \
		--module mjs:$(MJS_PATH) \
		--module arduino-drivers:$(REPO_ROOT)/arduino_drivers

$(MJS_PATH)/mjs.c $(MJS_PATH)/mjs.h:
	@echo "=== Creating $@"
	@make -C $(MJS_PATH) $(notdir $@)

clean:
	echo "TODO: add mgos clean"
	@for d in $(EXAMPLES) ; do \
	  rm -rf $$d/build; \
	done

list_examples:
	@echo $(EXAMPLES)
