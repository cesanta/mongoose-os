PLATFORM ?= esp8266

EXAMPLES_esp8266 = c_hello c_mcast c_mdns c_mqtt c_network js_hello
EXAMPLES_cc3200 = c_hello c_network c_mqtt js_hello

$(eval EXAMPLES=$(EXAMPLES_$(PLATFORM)))

.PHONY: all $(EXAMPLES) list_examples

export PLATFORM

all: $(EXAMPLES)

$(EXAMPLES):
	$(MAKE) -C $@ clean all

clean:
	@for d in $(EXAMPLES) ; do \
	  $(MAKE) -C $$d clean; \
	done

list_examples:
	@echo $(EXAMPLES)
