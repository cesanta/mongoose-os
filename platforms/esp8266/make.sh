#!/bin/sh

SDK=cesanta/esp8266-build-oss

V7DIR=$(dirname $(dirname $(dirname $PWD)))
DIR=/cesanta${PWD##$V7DIR}
exec docker run --rm -it -v $V7DIR:/cesanta ${SDK} /bin/bash -c "cd $DIR && make $@"
