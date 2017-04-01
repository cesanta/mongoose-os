#!./tools/docker-build-wrapper -t mos
#
# Construct a docker image that contains a built installation
# of Mongoose OS 'mos' tool.
#
# The constructed Docker image serves Mongoose OS IDE on HTTP port 1992.
#
# Run this docker image with:
#   docker run -v /dev/ttyUSB0:/dev/ttyUSB0 --privileged -p 1992:1992 mos
#
# Connect to your running mos using your web browser to request:
#   http://address-of-your-docker-host:1992/
#

#
# Start with the official Go container from Docker Store
#
FROM golang

#
# Install Debian dependencies of Mongoose OS
#
RUN apt-get update && apt-get install -y apt-utils && apt-get install -y libftdi-dev python-git && apt-get clean && rm -rf /var/lib/apt/lists/*

#
# Install Go tools and libraries needed by Mongoose OS
#
RUN go get github.com/kardianos/govendor
ADD . /go/src/cesanta.com
WORKDIR /go/src/cesanta.com/mos
RUN go get -d
RUN govendor fetch github.com/jteeuwen/go-bindata/go-bindata
RUN govendor fetch github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs

#
# Build Mongoose OS
#
RUN make install
ENV MOS_LISTEN_ADDR 0.0.0.0:1992
EXPOSE 1992
CMD ["/go/bin/mos"]