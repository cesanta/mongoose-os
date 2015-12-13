SDK ?= $(shell cat sdk.version)
SRC_DIR ?= $(realpath ../../..)
PORT ?= auto

.PHONY: all clean

all clean:
	docker run --rm -it -v $(SRC_DIR):/src $(SDK) \
		/bin/bash -c "\
			make -C /src/v7 v7.c && \
			make -C /src/mongoose/src && \
			make -C /src/smartjs/platforms/cc3200 -f Makefile.build $@ $(MAKEFLAGS) \
		"

ifeq ("$(PORT)", "auto")
PORT = $(shell ls -1 /dev/ttyUSB* | tail -n 1)
endif
flash:
	docker run --rm -it --privileged -v $(SRC_DIR):/src $(SDK) /bin/bash -c "\
		cd /usr/local/bin; \
		./cc3200prog $(PORT) /src/smartjs/platforms/cc3200/firmware/smartjs.bin \
	"

debug:
	docker run --rm -it --privileged -v $(SRC_DIR):/src $(SDK) \
		/bin/bash -c "cd /src/smartjs/platforms/cc3200 && tools/gdb.sh"
