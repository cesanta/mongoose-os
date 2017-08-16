#!/bin/bash

PACKAGE=$1
DISTR=$2

[ -z "${PACKAGE}" -o -z "${DISTR}" ] && { echo "Usage: $0 package distr"; exit 1; }

[ -z "${GNUPGHOME}" ] && GNUPGHOME=$HOME/.gnupg-cesantabot
[ -d "${GNUPGHOME}" ] || { echo "${GNUPGHOME} does not exist"; exit 1; }

OUTDIR="$HOME/tmp/out-${DISTR}"

set -x -e

# Note: always using Xenial because Zesty has problem with passphrase entry.
docker run -it --rm \
    -v ${GNUPGHOME}:/root/.gnupg \
    -v ${OUTDIR}:/work \
    docker.cesanta.com/ubuntu-golang:xenial \
    /bin/bash -l -c "\
        cd /work && \
        debsign *_source.changes && \
        dput ppa:mongoose-os/mos *_source.changes"
