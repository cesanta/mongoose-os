/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_ARDUINO_SPI_H_
#define CS_FW_SRC_ARDUINO_SPI_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_ARDUINO_API && MGOS_ENABLE_SPI

#include <stdint.h>

#include <Arduino.h>

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

struct SPISettings {
  SPISettings();
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode);
  uint32_t clock;
  uint8_t bit_order;
  uint8_t mode;
};

class SPIImpl {
 public:
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

  // void write(uint8_t data);
  void write16(uint16_t data);
  void write32(uint32_t data);
  void writeBytes(uint8_t *data, uint32_t size);

  /* mOS stuff. */
  SPIImpl(struct mgos_spi *spi = nullptr);
  void setSPI(struct mgos_spi *spi);

 private:
  struct mgos_spi *spi_;
};

extern SPIImpl SPI;

#endif /* MGOS_ENABLE_ARDUINO_API && MGOS_ENABLE_SPI */

#endif /* CS_FW_SRC_ARDUINO_SPI_H_ */
