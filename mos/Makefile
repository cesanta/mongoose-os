.PHONY: build clean deploy deploy-fwbuild deploy-mos-binary downloads generate install

TAG ?= latest

all: build

generate version/version.go version/version.json:
	@# If we are building a Debian package, use its version.
	@# Debian package versions look like this:
	@#   1.12+92e435b~xenial0 (mos) or
	@#   201708051141+e90a9bf~xenial0 (mos-latest).
	@# The corresponding changelog entry looks like this:
	@# mos-latest (201708051141+e90a9bf~xenial0) xenial; urgency=low
	@# The part before "+" becomes version, entire string is used as build id.
	@[ -f ../debian/changelog ] && { \
	  head -n 1 ../debian/changelog | cut -d '(' -f 2 | cut -d '+' -f 1 > version/version; \
	  head -n 1 ../debian/changelog | cut -d '(' -f 2 | cut -d ')' -f 1 > version/build_id; \
	} || rm -f version/version version/build_id
	@go install cesanta.com/vendor/github.com/jteeuwen/go-bindata/go-bindata
	@go install cesanta.com/vendor/github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs
	@go generate . ./version
	@gofmt -w bindata_assetfs.go
	@cat version/version.json
	@echo

build: generate
	go build -v -i

install: generate
	go install

linux: generate
	docker run -i --rm \
    -v $${GOPATH%%:*}/src:/go/src \
    -v $$(pwd):/out \
    -v $$(pwd)/gobuild-cache:/go/pkg \
    docker.cesanta.com/gobuild32 \
    -tags 'netgo' \
    -o /out/downloads/mos/linux/mos -tags no_libudev --ldflags '-extldflags "-static"' \
    cesanta.com/mos

mac: generate
	GOOS=darwin go build -o downloads/mos/mac/mos

win: generate
	GOOS=windows GOARCH=386 go build -o downloads/mos/win/mos.exe

downloads: linux mac win
	cp version/version.json downloads/mos/

deploy: deploy-fwbuild deploy-mos-binary

deploy-fwbuild:
	cd ../prod && \
    ansible-playbook fwbuild.yml -e build=yes -e mos_version_tag='$(TAG)' -t cloud-mos,fwbuild-instance

deploy-mos-binary:
	cd ../prod && \
    ansible-playbook website.yml -e build=yes -e mos_version_tag='$(TAG)' -t build_mos,mos


clean:
	rm -rf mos_* mos.exe mos gobuild-cache downloads/mos/{mac,linux,win} version/version.*
