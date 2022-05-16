DOCKERFILES = esp32-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp32
TOOLCHAIN_URL = https://github.com/espressif/crosstool-NG/releases/download/esp-2021r2-patch3/xtensa-esp32-elf-gcc8_4_0-esp-2021r2-patch3-linux-amd64.tar.gz
DOCKER_FLAGS = --build-arg=TOOLCHAIN_URL=$(TOOLCHAIN_URL) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp32-build: mgos_fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
