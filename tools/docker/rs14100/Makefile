DOCKERFILES = rs14100-build-private
RS14100_SDK = RS14100.NB0.WM.GENR.1.0.4
DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG) \
               --build-arg=RS14100_SDK=$(RS14100_SDK)
# Use ESP8266 config for now.
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/rs14100

include ../docker.mk

DISTR_PATH = ../distr

RS14100_SDK_ZIP = $(DISTR_PATH)/$(RS14100_SDK).zip

docker-pre-build-rs14100-build-private: $(RS14100_SDK_ZIP) \
                                        mgos_fw_meta.py serve_core \
                                        mklfs mkspiffs mkspiffs8
	rm -rf tmp && mkdir tmp
	unzip -q -d tmp $(RS14100_SDK_ZIP)
	cd tmp/$(RS14100_SDK) && patch -b -p 0 < ../../SDK.patch

$(RS14100_SDK_ZIP):
	@echo "Get $(notdir $@) from Redpine OpenKM and put in $(DISTR_PATH)"
	@exit 1

clean: clean-tools
	rm -rf tmp
