PLATFORM ?= esp8266

REPO_ROOT = $(abspath ../..)
MJS_PATH = $(REPO_ROOT)/mjs
LIBS_PATH = $(REPO_ROOT)/mos_libs

MOS ?= mos
EXAMPLES =

.PHONY: all list_examples

export PLATFORM

all: $(EXAMPLES)

clean:
	@true

list_examples:
	@echo $(EXAMPLES)
