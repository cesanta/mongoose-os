SRC_DIR ?= $(realpath ../../..)
DIR = $(shell bash -c 'echo -n /cesanta$${PWD\#\#$(SRC_DIR)}')

SDK_CAT = $(shell cat sdk.version)

.PHONY: all clean

# this hack is needed to make `-$(MAKEFLAGS)` always work (notice the dash).
# Otherwise, $(MAKEFLAGS) does not contain the flag `w` when `make` runs
# directly, but it does contain this flag when it runs as a submake.
#
# see:
# - http://www.gnu.org/software/make/manual/html_node/Options_002fRecursion.html
# - http://www.gnu.org/software/make/manual/html_node/_002dw-Option.html
MAKEFLAGS += w

all clean:
	@docker run --rm -it -v /$(SRC_DIR):/cesanta $(SDK_CAT) \
		//bin/bash -c "\
			if [ -d /cesanta/v7 ] ; then make -C /cesanta/v7 v7.c ; fi && \
			if [ -d /cesanta/mongoose ] ; then make -C /cesanta/mongoose mongoose.c mongoose.h ; fi && \
			cd $(DIR) && \
			make -f Makefile.build $@ -$(MAKEFLAGS) \
				&& python ./tools/showbreakdown.py \
		"


