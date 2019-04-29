#!/bin/bash

set -e -x

rm -f *.zip
curl -qsSLO https://github.com/mongoose-os-apps/bootloader/releases/download/latest/bootloader-rs14100.zip
unzip -p bootloader-*.zip bootloader-*/bootloader.0.bin > bootloader.bin
rm -f *.zip
