#!/bin/bash

PACKAGE=$1
DISTR=$2

[ -z "${PACKAGE}" -o -z "${DISTR}" ] && { echo "Usage: $0 package distr"; exit 1; }

set -x -e

# Make sure the script is called from the mongoose-os repo
ORIGIN="$(git --work-tree $(dirname 0) remote get-url origin)"
if ! [[ "${ORIGIN}" =~ mongoose-os$ ]]; then
  echo "You should run this script from mongoose-os repository, not from ${ORIGIN}"; exit 1;
fi

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

