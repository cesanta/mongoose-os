DOCKERFILES = stm32-build
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/stm32
DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG) \
               --build-arg=STM32CUBE_F2_DIR=$(STM32CUBE_F2_DIR) \
               --build-arg=STM32CUBE_F4_DIR=$(STM32CUBE_F4_DIR) \
               --build-arg=STM32CUBE_L4_DIR=$(STM32CUBE_L4_DIR) \
               --build-arg=STM32CUBE_F7_DIR=$(STM32CUBE_F7_DIR)

include ../docker.mk

DISTR_PATH = ../distr

EXCL_COMMON = '*/Documentation/*' '*/Examples/*' '*/Middlewares/ST/*' '*/Projects/*' '*/Utilities/*' '*/Drivers/STM32*_HAL_Driver/*.chm'
EXCL_MBEDTLS = '*/Middlewares/Third_Party/mbedTLS/*'
EXCL_LWIP = '*/Middlewares/Third_Party/LwIP/*'

docker-pre-build-stm32-build: $(DISTR_PATH)/en.stm32cubef2.zip \
                              $(DISTR_PATH)/en.stm32cubef4.zip \
                              $(DISTR_PATH)/en.stm32cubel4.zip \
                              $(DISTR_PATH)/en.stm32cubef7.zip \
                              mgos_fw_meta.py serve_core \
                              mklfs mkspiffs mkspiffs8
	rm -rf tmp && mkdir tmp
	unzip -q $(DISTR_PATH)/en.stm32cubef2.zip -d tmp -x $(EXCL_COMMON) $(EXCL_MBEDTLS) $(EXCL_LWIP)
	cd tmp/STM32Cube_FW_F2_* && patch -b -p 0 -b < ../../patches/cube_f2.patch
	$(eval STM32CUBE_F2_DIR = $$(shell cd tmp && ls -1 -d STM32Cube_FW_F2_*))
	unzip -q $(DISTR_PATH)/en.stm32cubef4.zip -d tmp -x $(EXCL_COMMON) $(EXCL_MBEDTLS)
	cd tmp/STM32Cube_FW_F4_* && patch -b -p 0 -b < ../../patches/cube_f4.patch
	$(eval STM32CUBE_F4_DIR = $$(shell cd tmp && ls -1 -d STM32Cube_FW_F4_*))
	unzip -q $(DISTR_PATH)/en.stm32cubel4.zip -d tmp -x $(EXCL_COMMON)
	cd tmp/STM32Cube_FW_L4_* && patch -b -p 0 -b < ../../patches/cube_l4.patch
	$(eval STM32CUBE_L4_DIR = $$(shell cd tmp && ls -1 -d STM32Cube_FW_L4_*))
	unzip -q $(DISTR_PATH)/en.stm32cubef7.zip -d tmp -x $(EXCL_COMMON) $(EXCL_MBEDTLS) $(EXCL_LWIP)
	cd tmp/STM32Cube_FW_F7_* && patch -b -p 0 -b < ../../patches/cube_f7.patch
	$(eval STM32CUBE_F7_DIR = $$(shell cd tmp && ls -1 -d STM32Cube_FW_F7_*))

$(DISTR_PATH)/en.stm32cubef2.zip:
	@echo "Please obtain $@ from http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef2.html"
	@exit 1

$(DISTR_PATH)/en.stm32cubef4.zip:
	@echo "Please obtain $@ from http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef4.html"
	@exit 1

$(DISTR_PATH)/en.stm32cubel4.zip:
	@echo "Please obtain $@ from http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubel4.html"
	@exit 1

$(DISTR_PATH)/en.stm32cubef7.zip:
	@echo "Please obtain $@ from http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef7.html"
	@exit 1

clean: clean-tools
	rm -rf tmp
