SDK ?= docker.cesanta.com:5000/cc3200-build:1.1.0
SRC_DIR ?= $(realpath ../../..)
PORT ?= /dev/ttyUSB1

.PHONY: all clean

all clean:
	docker run --rm -it -v ${SRC_DIR}:/src ${SDK} \
		/bin/bash -c "\
			make -C /src/v7 v7.c && \
			make -C /src/smartjs/platforms/cc3200 -f Makefile.build $@\
		"

flash:
	cd /home/rojer/cesanta/energia-0101E0016/hardware/tools/lm4f/bin; \
    ./cc3200prog $(PORT) $(CURDIR)/firmware/smartjs.bin
