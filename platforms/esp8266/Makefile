SRC_DIR ?= $(realpath ../../..)
DIR = $(shell bash -c 'echo -n /cesanta$${PWD\#\#$(SRC_DIR)}')

SDK_CAT = $(shell cat sdk.version)

.PHONY: all clean

all clean:
	@docker run --rm -it -v /$(SRC_DIR):/cesanta $(SDK_CAT) \
		//bin/bash -c "\
			if [ -d /cesanta/v7 ] ; then make -C /cesanta/v7 v7.c ; fi && \
			if [ -d /cesanta/mongoose/src ] ; then make -C /cesanta/mongoose/src ; fi && \
			cd $(DIR) && \
			make -f Makefile.build $@ -$(MAKEFLAGS) \
				&& python ./tools/showbreakdown.py \
		"


