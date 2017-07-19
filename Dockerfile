#!./tools/docker-build-wrapper -t mos-armhf
#
# Construct a docker image that contains a built installation
# of Mongoose OS 'mos' tool,
#
# The constructed Docker image serves Mongoose OS IDE on HTTP port 1992.
#
# Run this docker image with:
#   docker run -v /dev/ttyUSB0:/dev/ttyUSB0 --privileged -p 1992:1992 mos-armhf
#
# Connect to your running mos using your web browser to request:
#   http://address-of-your-docker-host:1992/
#

#
# As there is no offical armhf golang image, you must build one using
# Dockerfile-golang-armhf.
#
FROM golang

#
# Install Go tools and libraries needed by Mongoose OS
#
RUN apt-get update && \
	apt-get install -y --no-install-recommends libftdi-dev python-git && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*
RUN go get github.com/kardianos/govendor
ADD . /go/src/cesanta.com
WORKDIR /go/src/cesanta.com/mos
RUN govendor sync

#
# Build Mongoose OS
#
RUN make install
ENV MOS_LISTEN_ADDR 0.0.0.0:1992
EXPOSE 1992
CMD ["/go/bin/mos"]
