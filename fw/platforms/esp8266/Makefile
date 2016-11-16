APP ?= mongoose-iot
APP_PLATFORM = esp8266
ifeq "$(APP)" "mongoose-iot"
MIOT_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MIOT_PATH)/fw/docker_common.mk

DOCKER_MIOT_ESP_PATH = $(DOCKER_MIOT_PATH)/fw/platforms/esp8266

debug_coredump:
ifndef CONSOLE_LOG
	$(error Please set CONSOLE_LOG)
endif
	docker run --rm -i --tty=true \
	  -v $(APP_MOUNT_PATH):$(DOCKER_APP_PATH) \
	  -v $(MIOT_PATH_ABS):$(DOCKER_MIOT_PATH) \
	  -v $(MIOT_PATH_ABS):$(MIOT_PATH_ABS) \
	  -v $(CONSOLE_LOG):/console.log \
	  $(SDK_VERSION) /bin/bash -c "\
	  cd $(DOCKER_APP_PATH)/$(APP_SUBDIR); \
	    $(DOCKER_MIOT_ESP_PATH)/tools/serve_core.py \
	      --irom .build/fw_temp/0x11000.bin \
	      --rom $(DOCKER_MIOT_ESP_PATH)/tools/rom.bin \
	      /console.log & \
	    xt-gdb .build/$(APP).out \
	      -ex 'target remote 127.0.0.1:1234' \
	      -ex 'set confirm off' \
	      -ex 'add-symbol-file $(DOCKER_MIOT_ESP_PATH)/tools/romsyms 0x40000000'"
