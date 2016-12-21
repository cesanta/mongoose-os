PLATFORM ?= esp8266

REPO_ROOT = $(abspath ../..)

EXAMPLES_esp8266 = c_heater c_hello c_mcast c_mdns c_mqtt c_network js_hello c_websocket
EXAMPLES_esp32 = c_hello c_mqtt c_network c_websocket
EXAMPLES_cc3200 = c_hello c_network c_mqtt js_hello
EXAMPLES_stm32 = c_hello c_mqtt

MIOT ?= miot

$(eval EXAMPLES=$(EXAMPLES_$(PLATFORM)))

.PHONY: all $(EXAMPLES) list_examples

export PLATFORM

all: $(EXAMPLES)

$(EXAMPLES):
	@echo "=== Building $@ / $(PLATFORM)"
	@cd $@; $(MIOT) build --local --repo ${REPO_ROOT} --arch ${PLATFORM} --clean

clean:
	echo "TODO: add miot clean"
	@for d in $(EXAMPLES) ; do \
	  rm -rf $$d/build; \
	done

list_examples:
	@echo $(EXAMPLES)
