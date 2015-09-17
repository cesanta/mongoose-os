#!/bin/bash

cd $(dirname $0)

if [ "$1" = "RTOS" ]; then
    shift
    SDK=$(cat sdk.version)
else
    SDK=$(cat oss_sdk.version)
fi

V7DIR=$(dirname $(dirname $PWD))
DIR=/cesanta/${PWD##$V7DIR}

# Note: "//" in "//bin/bash" is a workaround for a boot2docker bug on Windows.
V=$(for i in "$@" ; do echo $i; done | xargs  -I% echo -n "'%' " )

exec docker run --rm -it -v /$V7DIR:/cesanta ${SDK} //bin/bash -c \
     "cd $DIR && make $V && python ./tools/showbreakdown.py"
