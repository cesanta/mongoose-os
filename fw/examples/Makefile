PLATFORM ?= esp8266

REPO_ROOT = $(abspath ../..)

EXAMPLES_cc3200 = c_hello c_network c_mqtt
EXAMPLES_esp32 = c_hello c_mqtt c_rpc c_websocket
EXAMPLES_esp8266 = c_heater c_hello c_mcast c_mdns c_mqtt c_network c_websocket
EXAMPLES_pic32 =
EXAMPLES_stm32 = c_heater c_hello c_http c_i2c c_mqtt c_mcast c_mdns c_network c_rpc c_websocket mjs_hello mjs_base

MOS ?= mos

$(eval EXAMPLES=$(EXAMPLES_$(PLATFORM)))

.PHONY: all $(EXAMPLES) list_examples

export PLATFORM

all: $(EXAMPLES)

$(EXAMPLES):
	@echo "=== Building $@ / $(PLATFORM)"
	@cd $@; $(MOS) build --local --repo ${REPO_ROOT} --arch ${PLATFORM} --clean

clean:
	echo "TODO: add mgos clean"
	@for d in $(EXAMPLES) ; do \
	  rm -rf $$d/build; \
	done

list_examples:
	@echo $(EXAMPLES)
