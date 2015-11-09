SDK ?= OSS

V7DIR ?= $(realpath ../..)
DIR = $(shell bash -c 'echo -n /cesanta$${PWD\#\#$(V7DIR)}')


ifeq ($(SDK),RTOS)
	SDK_CAT = $(shell cat sdk.version)
else
	SDK_CAT = $(shell cat oss_sdk.version)
endif


.PHONY: all clean

all clean:
	@docker run --rm -it -v /$(V7DIR):/cesanta $(SDK_CAT) \
		//bin/bash -c "\
			if [ -d /cesanta/v7 ] ; then make -C /cesanta/v7 v7.c ; fi && \
			if [ -d /cesanta/mongoose/src ] ; then make -C /cesanta/mongoose/src ; fi && \
			\
			cd $(DIR) && \
			make -f Makefile.build $@ CFLAGS_EXTRA='$(CFLAGS_EXTRA)' && \
			python ./tools/showbreakdown.py \
		"


