#!/bin/sh

if [ "$1" = "RTOS" ]; then
    shift
    SDK=$(cat sdk.version)
else
    SDK=$(cat oss_sdk.version)
fi

V7DIR=$(dirname $(dirname $PWD))
DIR=/cesanta/${PWD##$V7DIR}
# Note: "//" in "//bin/bash" is a workaround for a boot2docker bug on Windows.
exec docker run --rm -it -v /$V7DIR:/cesanta ${SDK} //bin/bash -c \
     "cd $DIR && make $@ && python ./tools/showbreakdown.py"
