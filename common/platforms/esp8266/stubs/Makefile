#
# Copyright (c) 2015 Cesanta Software Limited
# All rights reserved
#

STUB = stub_hello.c
LIBS =
PARAMS =
PORT = /dev/ttyUSB0

BUILD_DIR = build
STUB_ELF = $(BUILD_DIR)/$(patsubst %.c,%.elf,$(STUB))
STUB_JSON = $(BUILD_DIR)/$(patsubst %.c,%.json,$(STUB))
SDK = docker.cesanta.com:5000/esp8266-build-oss:1.5.0-r4
XT_CC = xtensa-lx106-elf-gcc

.PHONY: all clean run wrap

all: $(STUB_ELF)

$(STUB_ELF): $(STUB) $(LIBS)
	@echo "  CC   $^ -> $@"
	@[ -d $(BUILD_DIR) ] || mkdir $(BUILD_DIR)
	@docker run --rm -i -v $(CURDIR):/cesanta $(SDK) //bin/bash -c \
    "cd /cesanta && \
     $(XT_CC) -I/opt/Espressif/ESP8266_SDK -std=c99 -Wall -Werror -Os \
         -mtext-section-literals -mlongcalls -nostdlib -fno-builtin \
         -Wl,-static -Tstub.ld -o $@ $^"

wrap: $(STUB_JSON)

$(STUB_JSON): $(STUB_ELF) esptool.py
	@echo "  WRAP $< -> $@"
	@docker run --rm -i -v $(CURDIR):/cesanta $(SDK) //bin/bash -c \
    "cd /cesanta && ./esptool.py wrap_stub $< > $@"

run: $(STUB_JSON)
	@echo "  RUN  $< $(PARAMS) -> $(PORT)"
	@time ./esptool.py --port $(PORT) run_stub $< $(PARAMS)

clean:
	@rm -rf $(BUILD_DIR)
