DOCKERFILES = cc3220-build
SPIFFS_CONFIG_PATH = /cesanta/fw/platforms/cc32xx/include/spiffs

TI_COMPILER_DIR = ti-cgt-arm_16.9.6.LTS
SDK_DIR = simplelink_cc32xx_sdk_$(subst .,_,$(firstword $(subst -, ,$(DOCKER_TAG))))

MBEDTLS_BRANCH ?= cc3220_$(DOCKER_TAG)

include ../docker.mk

$(TI_COMPILER_DIR) $(SDK_DIR):
	@echo ERROR: You must have $@ in the current directory to build this image
	@exit 1

docker-pre-build-cc3220-toolchain: $(TI_COMPILER_DIR) $(SDK_DIR) \
                                   fw_meta.py serve_core.py mkspiffs mkspiffs8

docker-build-cc3220-toolchain: DOCKER_FLAGS = --build-arg=TI_COMPILER_DIR=$(TI_COMPILER_DIR) \
                                              --build-arg=SDK_DIR=$(SDK_DIR)

# Build our mbedTLS with the new toolchain.
mbedtls:
	git clone git@github.com:cesanta/mbedtls

docker-pre-build-cc3220-build: mbedtls docker-build-cc3220-toolchain
	rm -rf cs_mbedtls && mkdir -p cs_mbedtls/library && \
	(cd mbedtls && git checkout master && \
	 git fetch -f origin $(MBEDTLS_BRANCH):$(MBEDTLS_BRANCH) && \
	 git checkout $(MBEDTLS_BRANCH) && \
	 git reset --hard HEAD) && \
	rsync -a mbedtls/include/ cs_mbedtls/include/ && \
	docker run --rm -it \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.cesanta.com/cc3220-toolchain:$(DOCKER_TAG) /bin/bash -c "\
	    cd /opt/cs_mbedtls/library && make clean && make -j4 \
	    EXTRA_CFLAGS=-DMBEDTLS_X509_CA_CHAIN_ON_DISK \
	  " && \
	  cp -v mbedtls/library/*.a cs_mbedtls/library

docker-build-cc3220-build: DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)

clean:
	rm -rf mbedtls fw_meta.py serve_core.py mkspiffs mkspiffs8
