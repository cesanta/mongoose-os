#!/bin/sh

#
# This tool uploads a OTA-ready firmware directory to the blobstore
#
# NOTE OSX/Windows:
#
# The generated metadata file will contain references to that IP address.
# If you're hosting the cloud environment locally for testing purposes and
# you're using docker-machine, make sure you setup SSH forwarding as described
# in the readme and use your host IP address instead of your docker VM address
# to address the blobstore.
#

if [ $# -lt 2 ]; then
  echo "Usage: $0 fw_dir blob_store_url" >&2
  exit 1
fi

set -e

BASE_URL=$2

FW_DIR=${1}
UPDATES_ROOT=/updates
URL=http://${BASE_URL}${UPDATES_ROOT}

CURL=curl

FW_FILE=0x11000.bin
FS_FILE=0xe0000.bin
MANIFEST_FILE=manifest.json

for i in ${FW_DIR}/${FW_FILE} ${FW_DIR}/${FS_FILE} ${FW_DIR}/${MANIFEST_FILE}; do
    $CURL -i -F filedata=@$i ${URL}
done
