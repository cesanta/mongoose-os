e :=
c := ,
clist = $(subst $e $e,$c,$(strip $1))

DOCKERFILES = esp32-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp32
PLATFORMS = arm64 amd64

DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG) \
               --platform $(call clist, $(foreach i, $(PLATFORMS), linux/$(i)))

include ../docker.mk

docker-pre-build-esp32-build: mgos_fw_meta.py serve_core \
                              $(foreach i,$(PLATFORMS),mklfs-$(i)) \
                              $(foreach i,$(PLATFORMS),mkspiffs-$(i)) \
                              $(foreach i,$(PLATFORMS),mkspiffs8-$(i)) 
                               

clean: clean-tools
