# This provides a debug_coredump target.

OBJ_DIR ?= build/objs
ELF_FILE ?= $(OBJ_DIR)/fw.elf
EXTRA_GDB_ARGS ?=
EXTRA_SERVE_CORE_ARGS ?=

debug_coredump:
ifndef CONSOLE_LOG
	$(error Please set CONSOLE_LOG)
endif
	docker run --rm -i --tty=true \
	  -v $(APP_MOUNT_PATH):$(DOCKER_APP_PATH) \
	  -v $(MGOS_PATH_ABS):$(DOCKER_MGOS_PATH) \
	  -v $(MGOS_PATH_ABS):$(MGOS_PATH_ABS) \
	  -v $(realpath $(ELF_FILE)):/app.elf \
	  -v $(realpath $(CONSOLE_LOG)):/console.log \
	  $(SDK_VERSION) /bin/bash -c "\
	  cd $(DOCKER_APP_PATH)/$(APP_SUBDIR); \
	    $(DOCKER_MGOS_PATH)/common/tools/serve_core.py \
	      $(EXTRA_SERVE_CORE_ARGS) \
	      /app.elf /console.log & \
	    $(GDB) /app.elf \
	      -ex 'target remote 127.0.0.1:1234' \
	      -ex 'set confirm off' \
	      $(EXTRA_GDB_ARGS)"
