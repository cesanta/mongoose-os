/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/Arduino/mgos_arduino_spi.h"

#include <SPI.h>

#include "mongoose/mongoose.h"

#include "fw/src/mgos_spi.h"

SPIImpl SPI;

SPISettings::SPISettings() : clock(1000000), bit_order(MSBFIRST), mode(0) {
}

SPISettings::SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t mode)
    : clock(clock), bit_order(bitOrder), mode(mode) {
}

SPIImpl::SPIImpl(struct mgos_spi *spi) : spi_(spi) {
}

void SPIImpl::begin() {
}

void SPIImpl::end() {
}

void SPIImpl::setBitOrder(uint8_t bitOrder) {
  if (spi_ == nullptr) return;
  mgos_spi_set_msb_first(spi_, (bitOrder == MSBFIRST));
}

void SPIImpl::setDataMode(uint8_t dataMode) {
  if (spi_ == nullptr) return;
  mgos_spi_set_mode(spi_, dataMode);
}

void SPIImpl::setFrequency(uint32_t freq) {
  if (spi_ == nullptr) return;
  mgos_spi_set_freq(spi_, freq);
}

void SPIImpl::beginTransaction(SPISettings settings) {
  if (spi_ == nullptr) return;
  setFrequency(settings.clock);
  setBitOrder(settings.bit_order);
  setDataMode(settings.mode);
}

uint8_t SPIImpl::transfer(uint8_t data) {
  if (spi_ == nullptr) return 0;
  if (!mgos_spi_txn(spi_, &data, &data, 1)) data = 0;
  return data;
}

uint16_t SPIImpl::transfer16(uint16_t data) {
  if (spi_ == nullptr) return 0;
  data = htons(data);
  if (!mgos_spi_txn(spi_, &data, &data, 2)) data = 0;
  data = ntohs(data);
  return data;
}

uint32_t SPIImpl::transfer32(uint32_t data) {
  if (spi_ == nullptr) return 0;
  data = htonl(data);
  if (!mgos_spi_txn(spi_, &data, &data, 4)) data = 0;
  data = ntohl(data);
  return data;
}

void SPIImpl::transfer(void *data, size_t count) {
  if (spi_ == nullptr) return;
  mgos_spi_txn(spi_, data, data, count);
}

void SPIImpl::transferBytes(const uint8_t *data, uint8_t *out, uint32_t size) {
  if (spi_ == nullptr) return;
  mgos_spi_txn(spi_, data, out, size);
}

/*
void SPIImpl::write(uint8_t data) {
  if (spi_ == nullptr) return;
  mgos_spi_txn(spi_, &data, 1, NULL, 0);
}
*/

void SPIImpl::write16(uint16_t data) {
  if (spi_ == nullptr) return;
  data = htons(data);
  mgos_spi_txn_hd(spi_, &data, 2, NULL, 0);
}

void SPIImpl::write32(uint32_t data) {
  if (spi_ == nullptr) return;
  data = htonl(data);
  mgos_spi_txn_hd(spi_, &data, 4, NULL, 0);
}


void SPIImpl::setSPI(struct mgos_spi *spi) {
  spi_ = spi;
}

void mgos_arduino_spi_init(void) {
  SPI.setSPI(mgos_spi_get_global());
}
