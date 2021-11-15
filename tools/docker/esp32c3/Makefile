DOCKERFILES = esp32c3-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp32
TOOLCHAIN_VERSION = riscv32-esp-elf-gcc8_4_0-esp-2021r2-linux-amd64
DOCKER_FLAGS = --build-arg=TOOLCHAIN_VERSION=$(TOOLCHAIN_VERSION) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp32c3-build: mgos_fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
