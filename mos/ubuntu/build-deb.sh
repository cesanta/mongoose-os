#!/bin/bash

PACKAGE=$1
DISTR=$2

[ -z "${PACKAGE}" -o -z "${DISTR}" ] && { echo "Usage: $0 package distr"; exit 1; }

set -x -e

IMAGE=docker.cesanta.com/ubuntu-golang:${DISTR}

docker pull ${IMAGE}
docker run -i -t --rm \
    -v $PWD:/src \
    -v /tmp/out-${DISTR}:/tmp/work \
    ${IMAGE} \
    /bin/bash -l -c "\
        cd /src && rm -rf /tmp/work/* && \
        git-build-recipe --allow-fallback-to-native --package ${PACKAGE} --distribution ${DISTR} \
            /src/mos/ubuntu/${PACKAGE}-${DISTR}.recipe /tmp/work && \
        cd /tmp/work/${PACKAGE} && \
        debuild --no-tgz-check -us -uc -b"

