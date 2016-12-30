APP ?= mongoose-os
APP_PLATFORM = esp32
ifeq "$(APP)" "mongoose-os"
MGOS_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MGOS_PATH)/fw/docker_common.mk

# This actually overlaps with IRAM. But the ESP32 binary image includes both,
# so it's ok. Actual IRAM from the CD is then overlayed on top.
IROM_MAP_ADDR = 0x400a0000
GDB = xtensa-esp32-elf-gdb
include $(MGOS_PATH)/common/platforms/esp/debug_coredump.mk
