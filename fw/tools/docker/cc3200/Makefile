DOCKERFILES = cc3200-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/cc3200

# SDK cannot be downloaded directly (requires registering on the TI site).
# Furthermore, it is distributed as a Windows binary only (boo). So:
# 1) Go to http://www.ti.com/tool/CC3200SDK and download the SDK exe file.
# 2) "Install" the SDK (which just extracts a bunch of files).
#    You don't need Windows for this, Wine will do just fine.
# 3) The default install path is C:\TI\CC3200SDK_?.?.?
#    Take that entire directory and put here.
SDK_DIR = CC3200SDK_$(firstword $(subst -, ,$(DOCKER_TAG)))

# Get CCS and find it in $TI_DIR/ccsv*/tools/compiler/ti-cgt-arm_?.?.?
TI_COMPILER_DIR = ti-cgt-arm_16.9.6.LTS

# NWP service pack.
# These are from $TI_DIR/CC3100_CC3200_ServicePack_X.X.X.X-X.X.X.X/ota
# (names changed slightly).
NWP_SP_FILE = ota_1.0.1.11-2.8.0.0.ucf

MBEDTLS_BRANCH ?= cc3200_$(DOCKER_TAG)

include ../docker.mk

$(TI_COMPILER_DIR) $(SDK_DIR) $(NWP_SP_FILE) $(NWP_SP_FILE).sign:
	@echo ERROR: You must have $@ in the current directory to build this image
	@exit 1

docker-pre-build-cc3200-toolchain: $(TI_COMPILER_DIR) $(SDK_DIR) $(NWP_SP_FILE) $(NWP_SP_FILE).sign \
                                   fw_meta.py serve_core.py mklfs mkspiffs mkspiffs8

docker-build-cc3200-toolchain: DOCKER_FLAGS = --build-arg=TI_COMPILER_DIR=$(TI_COMPILER_DIR) \
                                              --build-arg=NWP_SP_FILE=$(NWP_SP_FILE) \
                                              --build-arg=SDK_DIR=$(SDK_DIR)

# Build our mbedTLS with the new toolchain.
mbedtls:
	git clone git@github.com:cesanta/mbedtls

docker-pre-build-cc3200-build: mbedtls docker-build-cc3200-toolchain
	rm -rf cs_mbedtls && mkdir -p cs_mbedtls/library && \
	(cd mbedtls && git checkout master && \
	 git fetch -f origin $(MBEDTLS_BRANCH):$(MBEDTLS_BRANCH) && \
	 git checkout $(MBEDTLS_BRANCH) && \
	 git reset --hard HEAD) && \
	rsync -a mbedtls/include/ cs_mbedtls/include/ && \
	docker run --rm -it \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.io/mgos/cc3200-toolchain:$(DOCKER_TAG) /bin/bash -c "\
	    cd /opt/cs_mbedtls/library && make clean && make -j4 \
	  " && \
	  cp -v mbedtls/library/*.a cs_mbedtls/library

docker-build-cc3200-build: DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)

clean:
	rm -rf mbedtls fw_meta.py serve_core.py mklfs mkspiffs mkspiffs8
