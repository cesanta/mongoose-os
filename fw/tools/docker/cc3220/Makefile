DOCKERFILES = cc3220-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/cc3220
DOCKER_FLAGS = --build-arg=TI_COMPILER_DIR=$(TI_COMPILER_DIR) \
               --build-arg=SDK_DIR=$(SDK_DIR) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)


TI_COMPILER_DIR = ti-cgt-arm_16.9.9.LTS
SDK_DIR = simplelink_cc32xx_sdk_$(subst .,_,$(firstword $(subst -, ,$(DOCKER_TAG))))

include ../docker.mk

$(TI_COMPILER_DIR) $(SDK_DIR):
	@echo ERROR: You must have $@ in the current directory to build this image
	@exit 1

docker-pre-build-cc3220-build: $(TI_COMPILER_DIR) $(SDK_DIR) \
                               fw_meta.py serve_core.py mklfs mkspiffs mkspiffs8

clean:
	rm -rf fw_meta.py serve_core.py mklfs mkspiffs mkspiffs8
