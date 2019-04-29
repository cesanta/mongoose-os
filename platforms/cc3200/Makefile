APP ?= mongoose-os
APP_PLATFORM = cc3200
ifeq "$(APP)" "mongoose-os"
MGOS_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MGOS_PATH)/fw/docker_common.mk

GDB = arm-none-eabi-gdb
include $(MGOS_PATH)/common/scripts/debug_coredump.mk
