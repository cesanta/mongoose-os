DOCKERFILES = rs14100-build
DOCKER_FLAGS = --build-arg=DOCKER_TAG=$(DOCKER_TAG)
# Use ESP8266 config for now.
SPIFFS_CONFIG_PATH = /vfs-fs-spiffs/include/rs14100

include ../docker.mk

DISTR_PATH = ../distr

docker-pre-build-rs14100-build: $(DISTR_PATH)/RS14100_DFP \
                                fw_meta.py serve_core.py \
                                mklfs mkspiffs mkspiffs8
	rm -rf tmp && mkdir tmp
	rsync -a $(DISTR_PATH)/RS14100_DFP/ tmp/RS14100_DFP/
	find tmp/RS14100_DFP/ -type d -print0 | xargs -0 chmod g+rx,o+rx
	find tmp/RS14100_DFP/ -type f -print0 | xargs -0 chmod u+w
	cd tmp/RS14100_DFP/1.0.4 && patch -p 0 < ../../../DFP.patch

$(DISTR_PATH)/RS14100_DFP:
	@echo "Get the DFP and put in $@ - download RS14100 device support pack for Keil and copy from there"
	@exit 1

clean:
	rm -rf fw_meta.py mklfs mkspiffs mkspiffs8 tmp
