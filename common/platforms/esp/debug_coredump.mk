# This provides a debug_coredump target for ESP8266 and ESP32.

OBJ_DIR ?= .build
BIN_FILE ?= $(OBJ_DIR)/$(APP).bin
ELF_FILE ?= $(OBJ_DIR)/$(APP).elf

debug_coredump:
ifndef CONSOLE_LOG
	$(error Please set CONSOLE_LOG)
endif
	docker run --rm -i --tty=true \
	  -v $(APP_MOUNT_PATH):$(DOCKER_APP_PATH) \
	  -v $(MGOS_PATH_ABS):$(DOCKER_MGOS_PATH) \
	  -v $(MGOS_PATH_ABS):$(MGOS_PATH_ABS) \
	  -v $(CONSOLE_LOG):/console.log \
	  $(SDK_VERSION) /bin/bash -c "\
	  cd $(DOCKER_APP_PATH)/$(APP_SUBDIR); \
	    $(DOCKER_MGOS_PATH)/common/platforms/esp/serve_core.py \
	      --rom $(DOCKER_MGOS_PATH)/common/platforms/$(APP_PLATFORM)/rom/rom.bin \
	      --irom $(BIN_FILE) --irom_addr $(IROM_MAP_ADDR) \
	      /console.log & \
	    $(GDB) $(ELF_FILE) \
	      -ex 'target remote 127.0.0.1:1234' \
	      -ex 'set confirm off' \
	      -ex 'add-symbol-file $(DOCKER_MGOS_PATH)/common/platforms/$(APP_PLATFORM)/rom/rom.elf 0x40000000'"
