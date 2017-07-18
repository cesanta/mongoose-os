.PHONY: generate

TAG ?= latest

all: generate

generate version.go version.json:
	@go install cesanta.com/vendor/github.com/jteeuwen/go-bindata/go-bindata
	@go install cesanta.com/vendor/github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs
	@go generate
	@gofmt -w bindata_assetfs.go
	@cat version.json
	@echo

install: generate
	go install

downloads: generate
	docker run -i --rm \
    -v $${GOPATH%%:*}/src:/go/src \
    -v $$(pwd):/out \
    -v $$(pwd)/gobuild-cache:/go/pkg \
    docker.cesanta.com/gobuild32 \
    -tags 'netgo' \
    -o /out/downloads/mos/linux/mos --ldflags '-extldflags "-static"' \
    cesanta.com/mos
	GOOS=darwin go build -o downloads/mos/mac/mos
	GOOS=windows GOARCH=386 go build -o downloads/mos/win/mos.exe
	cp version.json downloads/mos/

deploy: deploy-fwbuild deploy-mos-binary

deploy-fwbuild:
	cd ../prod && \
    ansible-playbook fwbuild.yml -e build=yes -e mos_version_tag='$(TAG)' -t cloud-mos,fwbuild-instance

deploy-mos-binary:
	cd ../prod && \
    ansible-playbook website.yml -e build=yes -e mos_version_tag='$(TAG)' -t build_mos,mos


clean:
	rm -rf mos_* mos.exe mos gobuild-cache downloads/mos/{mac,linux,win} version*
