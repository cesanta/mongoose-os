#!/bin/bash

set -e

make -C ../../../../../../common/platforms/esp8266/stubs clean wrap \
  STUB="../../esp/stub_flasher.c" LIBS="../../esp/slip.c uart.c" \
  STUB_JSON="${PWD}/data/stub_flasher_esp8266.json"
make -C ../../../../../../common/platforms/esp32/stubs clean wrap \
  STUB="../../esp/stub_flasher.c" LIBS="../../esp/slip.c spi_flash.c uart.c" \
  STUB_JSON="${PWD}/data/stub_flasher_esp32.json"
