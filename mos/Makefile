.PHONY: build clean deploy deploy-fwbuild deploy-mos-binary downloads generate install

TAG ?= latest

all: build

generate version/version.go version/version.json:
	@go install cesanta.com/vendor/github.com/jteeuwen/go-bindata/go-bindata
	@go install cesanta.com/vendor/github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs
	@go generate
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
    -o /out/downloads/mos/linux/mos --ldflags '-extldflags "-static"' \
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
