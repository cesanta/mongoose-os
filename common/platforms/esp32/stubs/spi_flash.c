// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * SPI magic required to get the flash chip into writable state.
 * Replaces stock ROM's SPIUnlock.
 *
 * Cargo-culted from
 *  https://github.com/espressif/esptool/blob/5c6962e894e0a118c9a4b5760876433493449260/flasher_stub/stub_write_flash.c
 */

#include "rom/spi_flash.h"
#include "soc/spi_reg.h"

static SpiFlashChip *flashchip = (SpiFlashChip *) 0x3ffae270;

#define STATUS_WIP_BIT (1 << 0) /* Write in Progress */
#define STATUS_QIE_BIT (1 << 9) /* Quad Enable */

#define SPI_IDX 1

/*
 * Wait for the SPI state machine to be ready,
 * ie no command in progress in the internal host.
 */
static void spi_wait_ready(void) {
  while (REG_READ(SPI_EXT2_REG(SPI_IDX)) & SPI_ST) {
  }
  while (REG_READ(SPI_EXT2_REG(0)) & SPI_ST) {
  }
}

/*
 * Returns true if the spiflash is ready for its next write operation.
 * Doesn't block, except for the SPI state machine to finish
 * any previous SPI host operation.
 */
static bool spiflash_is_ready(void) {
  spi_wait_ready();
  REG_WRITE(SPI_RD_STATUS_REG(SPI_IDX), 0);
  /* Issue read status command */
  REG_WRITE(SPI_CMD_REG(SPI_IDX), SPI_FLASH_RDSR);
  while (REG_READ(SPI_CMD_REG(SPI_IDX)) != 0) {
  }
  uint32_t status_value = REG_READ(SPI_RD_STATUS_REG(SPI_IDX));
  return (status_value & STATUS_WIP_BIT) == 0;
}

static void spiflash_wait_ready(void) {
  while (!spiflash_is_ready()) {
  }
}

static void spi_write_enable(void) {
  spiflash_wait_ready();
  REG_WRITE(SPI_CMD_REG(SPI_IDX), SPI_FLASH_WREN);
  while (REG_READ(SPI_CMD_REG(SPI_IDX)) != 0) {
  }
}

/*
 * Stub version of SPIUnlock() that replaces version in ROM.
 * This works around a bug where SPIUnlock sometimes reads the wrong
 * high status byte (RDSR2 result) and then copies it back to the
 * flash status, causing lock bit CMP or Status Register Protect ` to
 * become set.
 */
SpiFlashOpResult SPIUnlock(void) {
  uint32_t status;

  spi_wait_ready();

  if (SPI_read_status_high(&status) != SPI_FLASH_RESULT_OK) {
    return SPI_FLASH_RESULT_ERR;
  }

  /*
   * Clear all bits except QIE, if it is set.
   * (This is different from ROM SPIUnlock, which keeps all bits as-is.)
   */
  status &= STATUS_QIE_BIT;

  spi_write_enable();

  SET_PERI_REG_MASK(SPI_CTRL_REG(SPI_IDX), SPI_WRSR_2B);
  if (SPI_write_status(flashchip, status) != SPI_FLASH_RESULT_OK) {
    return SPI_FLASH_RESULT_ERR;
  }

  return SPI_FLASH_RESULT_OK;
}
