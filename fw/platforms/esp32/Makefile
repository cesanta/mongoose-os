APP ?= mongoose-iot
APP_PLATFORM = esp32
ifeq "$(APP)" "mongoose-iot"
MIOT_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MIOT_PATH)/fw/docker_common.mk

# This actually overlaps with IRAM. But the ESP32 binary image includes both,
# so it's ok. Actual IRAM from the CD is then overlayed on top.
IROM_MAP_ADDR = 0x400a0000
GDB = xtensa-esp32-elf-gdb
include $(MIOT_PATH)/common/platforms/esp/debug_coredump.mk
