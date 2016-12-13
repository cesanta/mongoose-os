# TODO(alex): move to docker

MKFILE_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))

all:
	@(cd $(MKFILE_DIR) && \
		$(MAKE) -f Makefile.build \
					BUILD_DIR=$(PWD)/${BUILD_DIR} \
					GEN_DIR=$(PWD)/${GEN_DIR} \
					APP_MODULES=$(PWD)/${APP_MODULES} \
					APP=${APP} \
					APP_CONF_SCHEMA=$(PWD)/${APP_CONF_SCHEMA} \
					MIOT_ENABLE_UPDATER=0 \
					MIOT_ENABLE_UPDATER_POST=0 \
					MIOT_ENABLE_UPDATER_RPC=0 \
					)
