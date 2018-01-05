.PHONY: build clean deploy deploy-fwbuild deploy-mos-binary downloads generate install dmg

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
	@go install cesanta.com/mos/vendor/github.com/jteeuwen/go-bindata/go-bindata
	@go install cesanta.com/mos/vendor/github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs
	@go generate . ./flash/cc3220 ./version
	@gofmt -w bindata_assetfs.go flash/cc3220/bindata.go
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
    docker.cesanta.com/gobuild32 \
    -tags 'netgo' \
    -o /out/downloads/mos/linux/mos -tags no_libudev --ldflags '-extldflags "-static"' \
    cesanta.com/mos

mac: generate
	brew install libftdi libusb-compat pkg-config
	go build -o downloads/mos/mac/mos


win: generate
	set -x ; docker run -i --rm \
    -v /$$(cd ../.. && pwd):/go/src \
    -v /$$(pwd):/out \
    docker.cesanta.com/gobuild-mingw \
    bash -c 'GOOS=windows GOARCH=386 CGO_ENABLED=1 CXX=i686-w64-mingw32-g++ CC=i686-w64-mingw32-gcc go build -o /out/downloads/mos/win/mos.exe --ldflags "-extldflags -static" cesanta.com/mos'

downloads: linux mac win dmg
	cp version/version.json downloads/mos/

deploy: deploy-fwbuild deploy-mos-binary

deploy-fwbuild: generate
	cd ../prod && \
    ansible-playbook fwbuild.yml -e build=yes -e mos_version_tag='$(TAG)' -t cloud-mos,fwbuild-instance

deploy-mos-binary: generate
	cd ../prod && \
    ansible-playbook website.yml -e build=yes -e mos_version_tag='$(TAG)' -t build_mos,mos

APPNAME = mos
DMGDIR = dmg/$(APPNAME).app
dmg: mac
	rm -rf dmg a.dmg mos.dmg
	mkdir -p $(DMGDIR)/Contents/{MacOS,Resources}
	install -m 644 $(APPNAME).icns $(DMGDIR)/Contents/Resources/
	install -m 644 Info.plist $(DMGDIR)/Contents/
	install -m 755 downloads/mos/mac/mos $(DMGDIR)/Contents/MacOS/_$(APPNAME)
	install -m 755 downloads/mos/libusb-1.0.0.dylib $(DMGDIR)/Contents/MacOS/
	install -m 755 downloads/mos/libftdi1.2.dylib  $(DMGDIR)/Contents/MacOS/
	install -m 755 mac_app_wrapper.sh $(DMGDIR)/Contents/MacOS/$(APPNAME)
	ln -fs /Applications dmg/
	hdiutil create a.dmg -volname mos -srcfolder dmg -ov
	mkdir -p downloads/mos/dmg
	hdiutil convert a.dmg -format UDRW -o downloads/mos/dmg/mos.dmg
	rm -rf a.dmg

clean:
	rm -rf mos_* mos.exe mos gobuild-cache downloads/mos/{mac,linux,win} version/version.*
