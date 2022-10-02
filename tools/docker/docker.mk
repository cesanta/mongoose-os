# Makefiles including this should set the DOCKERFILES var
# to a list of image names.
#
# For each image $(i) the following targets will be defined:
#
# docker-build-$(i): builds the image from Dockerfile-$(i)
# docker-push-$(i): pushes the image as $(REGISTRY)/$(i)

REPO_PATH ?= $(realpath ../../..)
REGISTRY ?= docker.io/mgos
PLATFORMS ?= amd64
DOCKER_TAG ?= latest
DOCKER_FLAGS ?= 
GCC ?= docker.io/gcc

# helper to substitute space with comma
e :=
c := ,
clist = $(subst $e $e,$c,$(strip $1))


all: docker-build

docker-build: $(foreach i,$(DOCKERFILES),docker-build-$(i))
docker-push: $(foreach i,$(DOCKERFILES),docker-push-$(i))

docker-pre-build-%: Dockerfile-%
	@true

docker-build-%: docker-pre-build-%
	@echo Building $(REGISTRY)/$*:$(DOCKER_TAG) "$(TOOLCHAIN_URL)"
	docker buildx build --load \
	--platform $(call clist, $(foreach i, $(PLATFORMS), linux/$(i))) \
	$(DOCKER_FLAGS) -t $(REGISTRY)/$*:$(DOCKER_TAG) -f Dockerfile-$* .
	@echo Built $(REGISTRY)/$*:$(DOCKER_TAG)

docker-push-%:
	docker buildx build --push \
	--platform $(call clist, $(foreach i, $(PLATFORMS), linux/$(i))) \
	$(DOCKER_FLAGS) -t $(REGISTRY)/$*:$(DOCKER_TAG) -f Dockerfile-$* .

docker-load-%:
	docker buildx build --load \
	$(DOCKER_FLAGS) -t $(REGISTRY)/$*:$(DOCKER_TAG) -f Dockerfile-$* .

mgos_fw_meta.py: $(REPO_PATH)/tools/mgos_fw_meta.py
	cp -v $< .

serve_core: $(wildcard $(REPO_PATH)/fw/tools/serve_core/*.py)
	rsync -av $(REPO_PATH)/tools/serve_core/ $@/

mklfs: $(foreach i,$(PLATFORMS),mklfs-$(i))

mklfs-%:
	rm -rf vfs-fs-lfs
	mkdir -p $*
	git clone --depth=1 https://github.com/mongoose-os-libs/vfs-fs-lfs
	docker run --rm -it \
		--platform linux/$* \
	  -v $(REPO_PATH):/mongoose-os \
	  -v $(CURDIR)/vfs-fs-lfs:/vfs-fs-lfs \
	  $(GCC) \
	  bash -c 'make -C /vfs-fs-lfs/tools -j4 mklfs unlfs \
	    FROZEN_PATH=/mongoose-os/src/frozen'
	cp -v vfs-fs-lfs/tools/mklfs vfs-fs-lfs/tools/unlfs $*/

mkspiffs mkspiffs8 : $(foreach i,$(PLATFORMS),mkspiffs-$(i)) $(foreach i,$(PLATFORMS),mkspiffs8-$(i))

mkspiffs-% mkspiffs8-%:
	rm -rf vfs-fs-spiffs
	mkdir -p $*
	git clone --depth=1 https://github.com/mongoose-os-libs/vfs-fs-spiffs
	docker run --rm -it \
		--platform linux/$* \
	  -v $(REPO_PATH):/mongoose-os \
	  -v $(CURDIR)/vfs-fs-spiffs:/vfs-fs-spiffs \
	  $(GCC) \
	  bash -c 'make -C /vfs-fs-spiffs/tools -j4 mkspiffs mkspiffs8 unspiffs unspiffs8 \
	    FROZEN_PATH=/mongoose-os/src/frozen \
	    SPIFFS_CONFIG_PATH=$(SPIFFS_CONFIG_PATH)'
	cp -v vfs-fs-spiffs/tools/mkspiffs vfs-fs-spiffs/tools/mkspiffs8 $*/
	cp -v vfs-fs-spiffs/tools/unspiffs vfs-fs-spiffs/tools/unspiffs8 $*/

clean-tools:
	rm -rf amd64 arm64 mgos_fw_meta.py serve_core serve_core.py mklfs mkspiffs mkspiffs8 vfs-fs-*
