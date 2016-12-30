APP ?= mongoose-os
APP_PLATFORM = pic32
ifeq "$(APP)" "mongoose-os"
MGOS_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MGOS_PATH)/fw/docker_common.mk

debug:
	docker run --rm -it --privileged -v $(SRC_DIR):/src $(SDK) \
		/bin/bash -c "cd /src/fw/platforms/pic32 && tools/gdb.sh"
