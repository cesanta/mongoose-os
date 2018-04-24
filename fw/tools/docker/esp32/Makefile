DOCKERFILES = esp32-build esp32-toolchain
SPIFFS_CONFIG_PATH = /cesanta/fw/platforms/esp32/include
TOOLCHAIN_VERSION = 1.22.0-80-g6c4433a-5.2.0
MBEDTLS_BRANCH ?= esp32_$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp32-toolchain: fw_meta.py serve_core.py mkspiffs mkspiffs8
docker-build-esp32-toolchain: DOCKER_FLAGS = --build-arg=TOOLCHAIN_VERSION=$(TOOLCHAIN_VERSION) \
                                             --build-arg=DOCKER_TAG=$(DOCKER_TAG)

# Build our mbedTLS with the new toolchain, with and without ATCA support.
docker-pre-build-esp32-build: mbedtls cryptoauthlib docker-build-esp32-toolchain
	rm -rf ../../fw/platforms/esp32/.build && \
	rm -rf cs_mbedtls && mkdir -p cs_mbedtls/port && \
	(cd mbedtls && git checkout master && \
	 git fetch -f origin $(MBEDTLS_BRANCH):$(MBEDTLS_BRANCH) && \
	 git checkout $(MBEDTLS_BRANCH) && \
	 git reset --hard HEAD) && \
	rsync -a mbedtls/include/ cs_mbedtls/include/ && \
	rsync -a mbedtls/port/include/ cs_mbedtls/port/include/ && \
	cp -v mbedtls/component.mk cs_mbedtls/ && \
	docker run --rm -it \
	  -v $(REPO_PATH):/opt/src \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.cesanta.com/esp32-toolchain:$(DOCKER_TAG) /bin/bash -c "\
	    nice make -j 4 -C /opt/src/fw/platforms/esp32 -f Makefile.build \
	      /opt/src/fw/platforms/esp32/.build/cs_mbedtls/libcs_mbedtls.a \
	      APP=test APP_VERSION=1.0 APP_BUILD_ID=dummy MGOS_PATH=/opt/src \
	      MGOS_BUILD_CS_MBEDTLS=1 MGOS_HAVE_ATCA=1 \
	  " && \
	cp -v $(REPO_PATH)/fw/platforms/esp32/.build/cs_mbedtls/libcs_mbedtls.a cs_mbedtls/libcs_mbedtls_atca.a && \
	rm -rf $(REPO_PATH)/fw/platforms/esp32/.build && \
	docker run --rm -it \
	  -v $(REPO_PATH):/opt/src \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.cesanta.com/esp32-toolchain:$(DOCKER_TAG) /bin/bash -c "\
	    nice make -j 4 -C /opt/src/fw/platforms/esp32 -f Makefile.build \
	      /opt/src/fw/platforms/esp32/.build/cs_mbedtls/libcs_mbedtls.a \
	      APP=test APP_VERSION=1.0 APP_BUILD_ID=dummy MGOS_PATH=/opt/src \
	      MGOS_BUILD_CS_MBEDTLS=1 \
	  " && \
	cp -v $(REPO_PATH)/fw/platforms/esp32/.build/cs_mbedtls/libcs_mbedtls.a cs_mbedtls/libcs_mbedtls.a && \
	rm -rf $(REPO_PATH)/fw/platforms/esp32/.build

docker-build-esp32-build: DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)

mbedtls:
	git clone git@github.com:cesanta/mbedtls

cryptoauthlib:
	rsync -a ../../third_party/cryptoauthlib .

clean:
	rm -rf cryptoauthlib mbedtls cs_mbedtls fw_meta.py serve_core.py mkspiffs mkspiffs8
