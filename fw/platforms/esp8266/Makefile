APP ?= mongoose-iot
APP_PLATFORM = esp8266
ifeq "$(APP)" "mongoose-iot"
MIOT_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MIOT_PATH)/fw/docker_common.mk

debug_coredump:
ifndef CONSOLE_LOG
	$(error Please set CONSOLE_LOG)
endif
	docker run --rm -i --tty=true \
	  -v $(REPO_ABS_PATH):/src \
	  -v $(CONSOLE_LOG):/console.log \
	  $(SDK_VERSION) /bin/bash -c "\
	    cd /src$(APP_SUBDIR); \
      /src$(MIOT_PLATFORM_SUBDIR)/tools/serve_core.py \
        --irom .build/fw_temp/0x11000.bin \
        --rom /src$(MIOT_PLATFORM_SUBDIR)/tools/rom.bin \
        /console.log & \
      xt-gdb .build/$(APP).out \
        -ex 'target remote 127.0.0.1:1234' \
        -ex 'set confirm off' \
        -ex 'add-symbol-file /src$(MIOT_PLATFORM_SUBDIR)/tools/romsyms 0x40000000'"
