#!/bin/bash

set -e

make -C ../../../common/platforms/esp32/stubs clean wrap \
  STUB="../../esp/stub_flasher.c" LIBS="../../esp/slip.c spi_flash.c uart.c" \
  STUB_JSON="${PWD}/data/stub_flasher.json"
