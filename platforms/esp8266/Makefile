APP ?= mongoose-os
APP_PLATFORM = esp8266
ifeq "$(APP)" "mongoose-os"
MGOS_PATH = ../../..
endif

# This defines "all" and "clean" targets.
include $(MGOS_PATH)/fw/docker_common.mk

GDB = xtensa-lx106-elf-gdb
EXTRA_GDB_ARGS = -ex 'add-symbol-file /opt/Espressif/rom/rom.elf 0x40000000'
EXTRA_SERVE_CORE_ARGS = --rom=/opt/Espressif/rom/rom.bin --rom_addr=0x40000000
include $(MGOS_PATH)/common/scripts/debug_coredump.mk
