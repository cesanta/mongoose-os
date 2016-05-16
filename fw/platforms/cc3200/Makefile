SDK ?= $(shell cat sdk.version)
SRC_DIR ?= $(realpath ../../..)

.PHONY: all clean

MAKEFLAGS += w

all clean:
	docker run --rm -i -v $(SRC_DIR):/src $(SDK) \
		/bin/bash -c "\
			make -C /src/v7 v7.c && \
			make -C /src/mongoose mongoose.c mongoose.h && \
			make -C /src/fw/platforms/cc3200 -f Makefile.build $@ -$(MAKEFLAGS) \
		"

debug:
	docker run --rm -it --privileged -v $(SRC_DIR):/src $(SDK) \
		/bin/bash -c "cd /src/fw/platforms/cc3200 && tools/gdb.sh"
