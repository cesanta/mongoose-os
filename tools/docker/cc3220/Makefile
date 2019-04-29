DOCKERFILES = cc3220-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/cc3220
DOCKER_FLAGS = --build-arg=TI_COMPILER_DIR=$(TI_COMPILER_DIR) \
               --build-arg=SDK_DIR=$(SDK_DIR) \
               --build-arg=DOCKER_TAG=$(DOCKER_TAG)

DISTR_PATH = ../distr

TI_COMPILER_DIR = ti-cgt-arm_16.9.9.LTS
SDK_DIR = simplelink_cc32xx_sdk_2_10_00_04

include ../docker.mk

$(DISTR_PATH)/$(TI_COMPILER_DIR) $(DISTR_PATH)/$(SDK_DIR):
	@echo ERROR: Please obtain $@ to build this image
	@exit 1

docker-pre-build-cc3220-build: $(DISTR_PATH)/$(TI_COMPILER_DIR) $(DISTR_PATH)/$(SDK_DIR) \
                               mgos_fw_meta.py serve_core mklfs mkspiffs mkspiffs8
	rm -rf tmp && mkdir tmp
	rsync -a $(DISTR_PATH)/$(TI_COMPILER_DIR)/ tmp/$(TI_COMPILER_DIR)/
	cd tmp/$(TI_COMPILER_DIR)/lib && patch -b -p 0 < ../../../ti_libc.patch
	rsync -a $(DISTR_PATH)/$(SDK_DIR)/ tmp/$(SDK_DIR)/
	cd tmp/$(SDK_DIR) && patch -b -p 0 < ../../sdk.patch
	# There is List.c in the SDK and then there's list.c in FreeRTOS.
	# MacOS, in its infinite wisdom, confuses the two. Hilarity ensues.
	mv tmp/$(SDK_DIR)/source/ti/drivers/utils/List.c \
	   tmp/$(SDK_DIR)/source/ti/drivers/utils/ListUtils.c
	rsync -a user.h tmp/$(SDK_DIR)/source/ti/drivers/net/wifi/porting/user.h

clean: clean-tools
	rm -rf tmp
