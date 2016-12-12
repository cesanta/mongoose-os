# TODO(alex): move to docker

MKFILE_DIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))

all:
	@(cd $(MKFILE_DIR) && \
		$(MAKE) -f Makefile.build \
					BUILD_DIR=$(PWD)/${BUILD_DIR} \
					GEN_DIR=$(PWD)/${GEN_DIR} \
					APP_MODULES=$(PWD)/${APP_MODULES} \
					)
