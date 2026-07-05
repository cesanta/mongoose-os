# DRAFT: build the mgos/esp32c6-build SDK image.
# Usage: make            (builds docker.io/mgos/esp32c6-build:6.0.2-r0)
#        make docker-push-esp32c6-build

DOCKERFILES = esp32c6-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp32xx
PLATFORMS = amd64
# Can't build arm64 image on amd64. TODO(nikwest): Fix it.
# arm64

# Image tag (mgos SDK revision) and the upstream ESP-IDF tag baked into it.
DOCKER_TAG ?= 6.0.2-r0
IDF_VERSION ?= v6.0.2

DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG) \
               --build-arg=IDF_VERSION=$(IDF_VERSION)

include ../docker.mk

docker-pre-build-esp32c6-build: mgos_fw_meta.py serve_core mklfs mkspiffs mkspiffs8

clean: clean-tools
