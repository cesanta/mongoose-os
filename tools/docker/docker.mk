# Makefiles including this should set the DOCKERFILES var
# to a list of image names.
#
# For each image $(i) the following targets will be defined:
#
# docker-build-$(i): builds the image from Dockerfile-$(i)
# docker-push-$(i): pushes the image as $(REGISTRY)/$(i)

REPO_PATH ?= $(realpath ../../..)
REGISTRY ?= docker.io/mgos
DOCKER_TAG ?= latest
DOCKER_FLAGS ?=

all: docker-build

docker-build: $(foreach i,$(DOCKERFILES),docker-build-$(i))
docker-push: $(foreach i,$(DOCKERFILES),docker-push-$(i))

docker-pre-build-%: Dockerfile-%
	@true

docker-build-%: docker-pre-build-%
	@echo Building $(REGISTRY)/$*:$(DOCKER_TAG)
	docker build $(DOCKER_FLAGS) -t $(REGISTRY)/$*:$(DOCKER_TAG) -f Dockerfile-$* .
	@echo Built $(REGISTRY)/$*:$(DOCKER_TAG)

docker-push-%:
	docker push $(REGISTRY)/$*:$(DOCKER_TAG)

mgos_fw_meta.py: $(REPO_PATH)/tools/mgos_fw_meta.py
	cp -v $< .

serve_core: $(wildcard $(REPO_PATH)/fw/tools/serve_core/*.py)
	rsync -av $(REPO_PATH)/tools/serve_core/ $@/

mklfs:
	rm -rf vfs-fs-lfs
	git clone --depth=1 https://github.com/mongoose-os-libs/vfs-fs-lfs
	docker run --rm -it \
	  -v $(REPO_PATH):/mongoose-os \
	  -v $(CURDIR)/vfs-fs-lfs:/vfs-fs-lfs \
	  docker.io/mgos/gcc \
	  bash -c 'make -C /vfs-fs-lfs/tools mklfs \
	    FROZEN_PATH=/mongoose-os/src/frozen'
	cp -v vfs-fs-lfs/tools/$@ $@

mkspiffs mkspiffs8:
	rm -rf vfs-fs-spiffs
	git clone --depth=1 https://github.com/mongoose-os-libs/vfs-fs-spiffs
	docker run --rm -it \
	  -v $(REPO_PATH):/mongoose-os \
	  -v $(CURDIR)/vfs-fs-spiffs:/vfs-fs-spiffs \
	  docker.io/mgos/gcc \
	  bash -c 'make -C /vfs-fs-spiffs/tools mkspiffs mkspiffs8 \
	    FROZEN_PATH=/mongoose-os/src/frozen \
	    SPIFFS_CONFIG_PATH=$(SPIFFS_CONFIG_PATH)'
	cp -v vfs-fs-spiffs/tools/$@ $@

clean-tools:
	rm -rf mgos_fw_meta.py serve_core serve_core.py mklfs mkspiffs mkspiffs8 vfs-fs-*
