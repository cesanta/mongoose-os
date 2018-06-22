DOCKERFILES = esp8266-build esp8266-toolchain
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/esp8266
MBEDTLS_BRANCH ?= esp8266_$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-esp8266-toolchain: fw_meta.py serve_core.py mkspiffs mkspiffs8
docker-build-esp32-toolchain: DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)

# Build our mbedTLS with the new toolchain, with and without ATCA support.
docker-pre-build-esp8266-build: mbedtls cryptoauthlib docker-build-esp8266-toolchain
	rm -rf ../../fw/platforms/esp32/.build && \
	rm -rf cs_mbedtls && mkdir -p cs_mbedtls/library && \
	(cd mbedtls && git checkout master && \
	 git fetch -f origin $(MBEDTLS_BRANCH):$(MBEDTLS_BRANCH) && \
	 git checkout $(MBEDTLS_BRANCH) && \
	 git reset --hard HEAD) && \
	rsync -a mbedtls/include/ cs_mbedtls/include/ && \
	docker run --rm -it \
	  -v $(realpath $(REPO_PATH)):/opt/src \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.io/mgos/esp8266-toolchain:$(DOCKER_TAG) /bin/bash -c "\
	    make -C /opt/cs_mbedtls/library clean && \
	    nice make -j 4 -C /opt/src/fw/platforms/esp8266 -f Makefile.build \
	      /opt/cs_mbedtls/library/libmbedcrypto_atca.a \
	      /opt/cs_mbedtls/library/libmbedtls_atca.a \
	      /opt/cs_mbedtls/library/libmbedx509_atca.a \
	      APP=test APP_VERSION=1.0 APP_BUILD_ID=dummy MGOS_PATH=/opt/src \
	      MGOS_ENABLE_ATCA=1 MGOS_HAVE_ATCA=1 \
	  " && \
	  cp -v mbedtls/library/*.a cs_mbedtls/library && \
	docker run --rm -it \
	  -v $(realpath $(REPO_PATH)):/opt/src \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.io/mgos/esp8266-toolchain:$(DOCKER_TAG) /bin/bash -c "\
	    make -C /opt/cs_mbedtls/library clean && \
	    nice make -j 4 -C /opt/src/fw/platforms/esp8266 -f Makefile.build \
	      /opt/cs_mbedtls/library/libmbedcrypto.a \
	      /opt/cs_mbedtls/library/libmbedtls.a \
	      /opt/cs_mbedtls/library/libmbedx509.a \
	      APP=test APP_VERSION=1.0 APP_BUILD_ID=dummy MGOS_PATH=/opt/src \
	      MGOS_ENABLE_ATCA=0 MGOS_HAVE_ATCA=0 \
	  " && \
	  cp -v mbedtls/library/*.a cs_mbedtls/library

docker-build-esp8266-build: DOCKER_FLAGS = --build-arg DOCKER_TAG=$(DOCKER_TAG)

mbedtls:
	git clone git@github.com:cesanta/mbedtls

cryptoauthlib:
	rsync -a ../../third_party/cryptoauthlib .

clean:
	rm -rf cryptoauthlib mbedtls cs_mbedtls fw_meta.py serve_core.py mkspiffs mkspiffs8
