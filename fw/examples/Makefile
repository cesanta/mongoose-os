PLATFORM ?= esp8266

REPO_ROOT = $(abspath ../..)

EXAMPLES_esp8266 = c_heater c_hello c_mcast c_mdns c_mqtt c_network js_hello c_websocket
EXAMPLES_cc3200 = c_hello c_network c_mqtt js_hello

MIOT ?= miot

$(eval EXAMPLES=$(EXAMPLES_$(PLATFORM)))

.PHONY: all $(EXAMPLES) list_examples

export PLATFORM

all: $(EXAMPLES)

$(EXAMPLES):
	@cd $@; $(MIOT) build --local --repo ${REPO_ROOT} --arch ${PLATFORM} --clean

clean:
	echo "TODO: add miot clean"
	@for d in $(EXAMPLES) ; do \
	  rm -rf $$d/build; \
	done

list_examples:
	@echo $(EXAMPLES)
