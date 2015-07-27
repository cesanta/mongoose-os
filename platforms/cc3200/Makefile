SDK ?= docker.cesanta.com:5000/cc3200-build:1.1.0
SRC_DIR ?= $(realpath ../../..)

.PHONY: all

all clean:
	docker run --rm -it -v ${SRC_DIR}:/src ${SDK} \
		/bin/bash -c "\
			make -C /src/v7 v7.c && \
			make -C /src/smartjs/platforms/cc3200 -f Makefile.build $@\
		"
