#!/bin/sh

SDK=docker.cesanta.com:5000/eabihf-arm-build

DEVDIR=$(dirname $(dirname $PWD))
BASE=""

if [ -L ${DEVDIR}/src/platform.mk ]; then
    # symlink to common repo
    DEVDIR=$(dirname ${DEVDIR})
    BASE=smartjs/
fi

# Note: "//" in "//bin/bash" is a workaround for a boot2docker bug on Windows.
exec docker run --rm -it -v ${DEVDIR}:/cesanta ${SDK} //bin/bash -c "cd /cesanta/${BASE}platforms/posix && make $@"
