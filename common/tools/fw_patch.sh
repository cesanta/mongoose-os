#!/bin/bash

#
# Adds content of dir to the FS contained in fw.zip.
#

set -e

function usage() {
    echo "usage: $0 fw.zip outfw.zip dir"
    exit 1
}

FW=${1}
if [ ! -f "${FW}" ]; then
    usage
fi

OUTFW=${2}
if [ -z "${OUTFW}" ]; then
    usage
fi

DIR=${3}
if [ ! -d "${DIR}" ]; then
    usage
fi

# tools

MKTEMP=mktemp
if which gmktemp >/dev/null; then
    MKTEMP=gmktemp
fi
MKSPIFFS=mkspiffs
UNSPIFFS=unspiffs
FWMETA=$(realpath $(dirname $0)/fw_meta.py)

#

TMPDIR=$(${MKTEMP} -d)
function cleanup {
    rm -rf ${TMPDIR}
}
# trap cleanup EXIT

mkdir -p ${TMPDIR}/unpack
unzip -d ${TMPDIR}/unpack ${FW}

FWDIR=${TMPDIR}/unpack/*
FSBIN=${FWDIR}/$(${FWMETA} get ${FWDIR}/manifest.json 'parts.fs.src')
FSSIZE=$(wc -c <${FSBIN})

echo "${FSBIN} ${FSSIZE}"
ls -l ${FSBIN}

mkdir -p ${TMPDIR}/fs
${UNSPIFFS} -d ${TMPDIR}/fs ${FSBIN}
cp -a ${DIR}/* ${TMPDIR}/fs

${MKSPIFFS} ${FSSIZE} ${TMPDIR}/fs >${FSBIN}

SHA1=$(sha1sum ${FSBIN} | awk '{print $1}')
${FWMETA} set -i ${FWDIR}/manifest.json parts.fs.cs_sha1=${SHA1}

REALOUTFW=$(realpath ${OUTFW})
( cd ${FWDIR}; ${FWMETA} create_fw --manifest manifest.json --out ${REALOUTFW} )
