#!/bin/bash

PACKAGE=$1
DISTR=$2
VERSION=$3

[ -z "${PACKAGE}" -o -z "${DISTR}" ] && { echo "Usage: $0 <package> <distr> [<version>]"; exit 1; }

RECIPE_ORIG_FILE=$PWD/mos/ubuntu/${PACKAGE}-${DISTR}.recipe

if [ -z "${VERSION}" ]; then
  # Version was not given; use the recipe as it is
  RECIPE=/src/mos/ubuntu/${PACKAGE}-${DISTR}.recipe
  RECIPE_MOUNT=
else
  # Version is given: replace 1.11_CHANGE_ME with the actual version
  RECIPE_TMP_DIR=$HOME/tmp/recipe-${PACKAGE}-${DISTR}-${VERSION}
  mkdir -p ${RECIPE_TMP_DIR}
  RECIPE_HOST_FILE=${RECIPE_TMP_DIR}/recipe

  cat ${RECIPE_ORIG_FILE} | sed "s/1\.11_CHANGE_ME/${VERSION}/" > ${RECIPE_HOST_FILE}

  RECIPE=/recipe/recipe
  RECIPE_MOUNT=" -v ${RECIPE_TMP_DIR}:/recipe"
fi

set -x -e

# Make sure the script is called from the mongoose-os repo
ORIGIN="$(git --work-tree $(dirname 0) remote get-url origin)"
if ! [[ "${ORIGIN}" =~ mongoose-os(.git)?$ ]]; then
  echo "You should run this script from mongoose-os repository, not from ${ORIGIN}"; exit 1;
fi

IMAGE=docker.cesanta.com/ubuntu-golang:${DISTR}

mkdir -p $HOME/tmp

docker pull ${IMAGE}
docker run -i -t --rm \
    -v $PWD:/src \
    -v $HOME/tmp/out-${DISTR}:/tmp/work \
    ${RECIPE_MOUNT} \
    ${IMAGE} \
    /bin/bash -l -c "\
        cd /src && rm -rf /tmp/work/* && \
        git-build-recipe --allow-fallback-to-native --package ${PACKAGE} --distribution ${DISTR} \
            ${RECIPE} /tmp/work && \
        cd /tmp/work/${PACKAGE} && \
        debuild --no-tgz-check -us -uc -b"

