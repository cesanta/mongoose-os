DOCKERFILES = stm32-build
SPIFFS_CONFIG_PATH = /cesanta/fw/platforms/stm32/include/spiffs
MBEDTLS_BRANCH ?= stm32_$(DOCKER_TAG)

include ../docker.mk

STM32CUBE_F4_PATH = STM32Cube_FW_F4_$(STM32CUBE_F4_VERSION)
STM32CUBE_F7_PATH = STM32Cube_FW_F7_$(STM32CUBE_F4_VERSION)

docker-pre-build-stm32-toolchain: en.stm32cubef4.zip en.stm32cubef7.zip \
                                  fw_meta.py serve_core.py mkspiffs mkspiffs8
	rm -rf tmp && mkdir tmp
	unzip -q en.stm32cubef4.zip -x '*/Documentation/*' '*/Examples/*' '*/Middlewares/ST/*' '*/Projects/*' '*/Utilities/*' -d tmp
	cd tmp/STM32Cube_FW_F4_* && patch -p 0 -b < ../../lwipopts_f4.patch
	$(eval STM32CUBE_F4_DIR = $$(shell cd tmp && ls -1 -d STM32Cube_FW_F4_*))
	unzip -q en.stm32cubef7.zip -x '*/Documentation/*' '*/Examples/*' '*/Middlewares/ST/*' '*/Projects/*' '*/Utilities/*' -d tmp
	cd tmp/STM32Cube_FW_F7_* && patch -p 0 -b < ../../lwipopts_f7.patch
	$(eval STM32CUBE_F7_DIR = $$(shell cd tmp && ls -1 -d STM32Cube_FW_F7_*))

docker-build-stm32-toolchain: DOCKER_FLAGS = --build-arg=STM32CUBE_F4_DIR=$(STM32CUBE_F4_DIR) \
                                             --build-arg=STM32CUBE_F7_DIR=$(STM32CUBE_F7_DIR)

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
	  docker.io/mgos/stm32-toolchain:$(DOCKER_TAG) /bin/bash -c '\
	    cd /opt/cs_mbedtls/library && make clean && \
	    make -j4 CC=arm-none-eabi-gcc CFLAGS="-mthumb -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu99 -Os -ggdb -MD -Wall -Werror -Wundef -fno-common -fno-inline -ffunction-sections -fdata-sections"' && \
	  cp -v mbedtls/library/*.a cs_mbedtls/library \

en.stm32cubef4.zip:
	@echo "Please obtain $@ from http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef4.html"
	@exit 1

en.stm32cubef7.zip:
	@echo "Please obtain $@ from http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef7.html"
	@exit 1

docker-build-stm32-build: DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)

mbedtls:
	git clone git@github.com:cesanta/mbedtls

clean:
	rm -rf mbedtls cs_mbedtls fw_meta.py mkspiffs mkspiffs8
