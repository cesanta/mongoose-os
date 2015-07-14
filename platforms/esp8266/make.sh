#!/bin/sh

SDK=docker.cesanta.com:5000/esp8266-build-oss:1.2.0

V7DIR=$(dirname $(dirname $PWD))
DIR=/cesanta/${PWD##$V7DIR}
exec docker run --rm -it -v /$V7DIR:/cesanta ${SDK} //bin/bash -c "cd $DIR && rm -rf firmware && make $@ && python ./tools/showbreakdown.py"
