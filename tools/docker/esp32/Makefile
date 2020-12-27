DOCKERFILES = esp32-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp32
TOOLCHAIN_VERSION = xtensa-esp32-elf-gcc8_4_0-esp-2020r3-linux-amd64
DOCKER_FLAGS = --build-arg=TOOLCHAIN_VERSION=$(TOOLCHAIN_VERSION) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp32-build: mgos_fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
