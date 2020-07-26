DOCKERFILES = esp8266-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp8266
DOCKER_FLAGS = --build-arg TOOLCHAIN_TAG=toolchain-9.2.0-2.0.0-r2 --build-arg DOCKER_TAG=$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp8266-build: mgos_fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
