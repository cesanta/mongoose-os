APP ?= mongoose-os
APP_PLATFORM = esp8266
ifeq "$(APP)" "mongoose-os"
MGOS_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MGOS_PATH)/fw/docker_common.mk

DOCKER_MGOS_ESP_PATH = $(DOCKER_MGOS_PATH)/fw/platforms/esp8266

IROM_MAP_ADDR = 0x40211000
GDB = xtensa-lx106-elf-gdb
include $(MGOS_PATH)/common/platforms/esp/debug_coredump.mk
