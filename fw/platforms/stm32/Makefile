APP ?= mongoose-iot
APP_PLATFORM = stm32
ifeq "$(APP)" "mongoose-iot"
MIOT_PATH = ../../..
endif

.DEFAULT_GOAL := build_all

MKFILE_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))

include $(MIOT_PATH)/fw/docker_common.mk

DOCKER_PWD=$(subst $(APP_MOUNT_PATH),$(DOCKER_APP_PATH),$(PWD))
DOCKER_MKFILE_DIR=$(subst $(APP_MOUNT_PATH),$(DOCKER_APP_PATH),$(MKFILE_DIR))

MAKE_CMD = (cd $(DOCKER_MKFILE_DIR) && $(MAKE) -f Makefile.build \
					BUILD_DIR=$(DOCKER_PWD)/${BUILD_DIR} \
					GEN_DIR=$(DOCKER_PWD)/${GEN_DIR} \
					APP_MODULES=$(DOCKER_PWD)/${APP_MODULES} \
					APP=${APP} \
					APP_CONF_SCHEMA=$(DOCKER_PWD)/${APP_CONF_SCHEMA} \
					FW_DIR=$(DOCKER_PWD)/${FW_DIR} \
					APP_PLATFORM=${PLATFORM} \
					APP_FS_PATH=$(DOCKER_PWD)/${APP_FS_PATH} \
					)

build_all:
	@docker run --rm -i --tty=$T \
	  -v $(APP_MOUNT_PATH):$(DOCKER_APP_PATH) \
	  -v $(MIOT_PATH_ABS):$(DOCKER_MIOT_PATH) \
	  -v $(MIOT_PATH_ABS):$(MIOT_PATH_ABS) \
	  $(DOCKER_USER_ARG) \
	  $(DOCKER_EXTRA) $(SDK_VERSION) \
	/bin/bash -c "$(MAKE_CMD)"
