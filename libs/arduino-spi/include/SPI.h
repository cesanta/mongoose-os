/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_FW_SRC_ARDUINO_SPI_H_
#define CS_FW_SRC_ARDUINO_SPI_H_

#include "mgos_features.h"

#include <stdint.h>

#include "Arduino.h"

#include "mgos_spi.h"

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif

#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

#define SPI_HAS_TRANSACTION 1
#define SPI_DEFAULT_FREQ 1000000

struct SPISettings {
  SPISettings();
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode);
  uint32_t clock;
  uint8_t bit_order;
  uint8_t mode;
};

class SPIImpl {
 public:
  SPIImpl();
  ~SPIImpl();

  /* Arduino interface */
  void begin();
  void end();

  // void setHwCs(bool use);
  void setBitOrder(uint8_t bitOrder);
  void setDataMode(uint8_t dataMode);
  void setFrequency(uint32_t freq);
  // void setClockDivider(uint32_t clockDiv);

  void beginTransaction(SPISettings settings);
  void endTransaction(void);

  uint8_t transfer(uint8_t data);
  uint16_t transfer16(uint16_t data);
  uint32_t transfer32(uint32_t data);
  void transfer(void *data, size_t count);
  void transferBytes(const uint8_t *data, uint8_t *out, uint32_t size);
  // void transferBits(uint32_t data, uint32_t * out, uint8_t bits);

  void write(uint8_t data);
  void write16(uint16_t data);
  void write32(uint32_t data);
  void writeBytes(const uint8_t *data, uint32_t size);

 private:
  struct mgos_spi *spi_;
  struct mgos_spi_txn txn_;
};

extern SPIImpl SPI;

/* no_extern_c_check */

#endif /* CS_FW_SRC_ARDUINO_SPI_H_ */
