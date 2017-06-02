#!/bin/bash

set -e -x

make -C ../../../common/platforms/esp32/stubs clean wrap \
  STUB="../../esp/stub_flasher.c" \
  LIBS="../../esp/slip.c /opt/Espressif/esp-idf/components/spi_flash/spi_flash_rom_patch.c led.c uart.c" \
  STUB_JSON="${PWD}/data/stub_flasher.json"

go generate -v
