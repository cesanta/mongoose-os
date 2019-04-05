DOCKERFILES = esp32-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp32
TOOLCHAIN_VERSION = 1.22.0-80-g6c4433a-5.2.0
DOCKER_FLAGS = --build-arg=TOOLCHAIN_VERSION=$(TOOLCHAIN_VERSION) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp32-build: fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
