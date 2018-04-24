DOCKERFILES = stm32-build
SPIFFS_CONFIG_PATH = /cesanta/fw/platforms/stm32
MBEDTLS_BRANCH ?= stm32_$(DOCKER_TAG)

include ../docker.mk

docker-pre-build-stm32-toolchain: STM32Cube_FW_F7_V1.8.0.tar.xz \
                                  fw_meta.py serve_core.py mkspiffs mkspiffs8
docker-build-stm32-toolchain: DOCKER_FLAGS =

# Build our mbedTLS with the new toolchain, with and without ATCA support.
docker-pre-build-stm32-build: mbedtls docker-build-stm32-toolchain
	rm -rf cs_mbedtls && mkdir -p cs_mbedtls/library && \
	(cd mbedtls && git checkout master && \
	 git fetch -f origin $(MBEDTLS_BRANCH):$(MBEDTLS_BRANCH) && \
	 git checkout $(MBEDTLS_BRANCH) && \
	 git reset --hard HEAD) && \
	rsync -a mbedtls/include/ cs_mbedtls/include/ && \
	docker run --rm -it \
	  -v $(realpath mbedtls):/opt/cs_mbedtls \
	  --user $(shell id -u):$(shell id -u) \
	  docker.cesanta.com/stm32-toolchain:$(DOCKER_TAG) /bin/bash -c '\
	    cd /opt/cs_mbedtls/library && make clean && \
	    make -j4 CC=arm-none-eabi-gcc CFLAGS="-mthumb -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu99 -Os -ggdb -MD -Wall -Werror -Wundef -fno-common -fno-inline -ffunction-sections -fdata-sections"' && \
	  cp -v mbedtls/library/*.a cs_mbedtls/library \

STM32Cube_FW_F7_V1.8.0.tar.xz:
	@echo Please obtain STM32Cube from http://www.st.com/en/development-tools/stm32cubemx.html
	@echo Some heavy dependencies can be pruned:
	@echo   rm -rf Middlewares/ST Middlewares/Third_Party/mbedTLS Projects Utilities
	@exit 1

docker-build-stm32-build: DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)

mbedtls:
	git clone git@github.com:cesanta/mbedtls

clean:
	rm -rf mbedtls cs_mbedtls fw_meta.py mkspiffs mkspiffs8
