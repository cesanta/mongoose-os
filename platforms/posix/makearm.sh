#!/bin/sh

SDK=docker.cesanta.com:5000/eabihf-arm-build

DEVDIR=$(dirname $(dirname $(dirname $PWD)))

# Note: "//" in "//bin/bash" is a workaround for a boot2docker bug on Windows.
exec docker run --rm -it -v ${DEVDIR}:/cesanta ${SDK} //bin/bash -c "cd /cesanta/smartjs/platforms/posix && make $@"

