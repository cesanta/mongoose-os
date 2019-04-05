DOCKERFILES = esp8266-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp8266
DOCKER_FLAGS = --build-arg DOCKER_TAG=$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp8266-build: fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
