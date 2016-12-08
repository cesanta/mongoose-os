APP ?= mongoose-iot
APP_PLATFORM = esp8266
ifeq "$(APP)" "mongoose-iot"
MIOT_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MIOT_PATH)/fw/docker_common.mk

DOCKER_MIOT_ESP_PATH = $(DOCKER_MIOT_PATH)/fw/platforms/esp8266

IROM_MAP_ADDR = 0x40211000
GDB = xtensa-lx106-elf-gdb
include $(MIOT_PATH)/common/platforms/esp/debug_coredump.mk
