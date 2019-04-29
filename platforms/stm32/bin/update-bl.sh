#!/bin/bash

set -e -x

for b in B-L475E-IOT01A DISCO-F746NG Electron; do
  rm -f *.zip
  curl -qsSLO https://github.com/mongoose-os-apps/bootloader/releases/download/latest/bootloader-stm32-$b.zip
  unzip -p bootloader-*.zip bootloader-*/bootloader.bin > bootloader-$b.bin
  rm -f *.zip
done
