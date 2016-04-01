APP ?= smartjs
REPO_PATH ?= ../../..
SJS_ESP_PATH = $(REPO_PATH)/smartjs/platforms/esp8266
SDK_VERSION ?= $(shell cat $(SJS_ESP_PATH)/sdk.version)

REPO_ABS_PATH = $(realpath $(REPO_PATH))
APP_PATH = $(subst $(REPO_ABS_PATH),/cesanta,$(CURDIR))

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
	@docker run --rm -i -v $(REPO_ABS_PATH):/cesanta $(DOCKER_EXTRA) $(SDK_VERSION) \
		//bin/bash -c "\
			if [ -d /cesanta/v7 ] ; then make -C /cesanta/v7 v7.c ; fi && \
			if [ -d /cesanta/krypton ] ; then make -C /cesanta/krypton krypton.c ; fi && \
			if [ -d /cesanta/mongoose ] ; then make -C /cesanta/mongoose mongoose.c mongoose.h ; fi && \
			cd $(APP_PATH) && \
			make -f Makefile.build APP=$(APP) REPO_PATH=$(REPO_PATH) $@ -$(MAKEFLAGS) \
				&& python $(SJS_ESP_PATH)/tools/showbreakdown.py \
		"

debug_coredump:
ifndef CONSOLE_LOG
	$(error Please set CONSOLE_LOG)
endif
	docker run --rm -it -v $(REPO_ABS_PATH):/cesanta -v $(CONSOLE_LOG):/console.log $(SDK_VERSION) /bin/bash -c \
	  "cd $(APP_PATH); \
      $(SJS_ESP_PATH)/tools/serve_core.py \
        --irom firmware/0x11000.bin \
        --rom $(SJS_ESP_PATH)/tools/rom.bin \
        /console.log & \
      xt-gdb .build/$(APP).out \
        -ex 'target remote 127.0.0.1:1234' \
        -ex 'set confirm off' \
        -ex 'add-symbol-file $(SJS_ESP_PATH)/tools/romsyms 0x40000000'"
