#!/bin/sh

SDK=$(cat arm_sdk.version)
DEVDIR=$(dirname $(dirname $(dirname $PWD)))

# Note: "//" in "//bin/bash" is a workaround for a boot2docker bug on Windows.
exec docker run --rm -it -v ${DEVDIR}:/cesanta ${SDK} //bin/bash -c "cd /cesanta/fw/platforms/posix && make $@"
