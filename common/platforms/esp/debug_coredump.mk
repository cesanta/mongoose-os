# This provides a debug_coredump target for ESP8266 and ESP32.

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
	    $(DOCKER_MIOT_PATH)/common/platforms/esp/serve_core.py \
	      --rom $(DOCKER_MIOT_PATH)/common/platforms/$(APP_PLATFORM)/rom/rom.bin \
	      --irom .build/$(APP).bin --irom_addr $(IROM_MAP_ADDR) \
	      /console.log & \
	    $(GDB) .build/$(APP).elf \
	      -ex 'target remote 127.0.0.1:1234' \
	      -ex 'set confirm off' \
	      -ex 'add-symbol-file $(DOCKER_MIOT_PATH)/common/platforms/$(APP_PLATFORM)/rom/rom.elf 0x40000000'"
