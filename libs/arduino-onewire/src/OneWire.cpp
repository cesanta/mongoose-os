/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * Arduino 1-Wire API wrapper for compatibility
 *
 */

#include "OneWire.h"

OneWire::OneWire(uint8_t pin) {
  ow_ = mgos_onewire_create(pin);
}

OneWire::~OneWire() {
  mgos_onewire_close(ow_);
}

uint8_t OneWire::reset(void) {
  return mgos_onewire_reset(ow_);
}

void OneWire::write_bit(uint8_t v) {
  mgos_onewire_write_bit(ow_, v);
}

uint8_t OneWire::read_bit(void) {
  return mgos_onewire_read_bit(ow_);
}

void OneWire::write(uint8_t v, uint8_t power /* = 0 */) {
  mgos_onewire_write(ow_, v);
  (void) power;
}

void OneWire::write_bytes(const uint8_t *buf, uint16_t count,
                          bool power /* = 0 */) {
  mgos_onewire_write_bytes(ow_, buf, count);
  (void) power;
}

uint8_t OneWire::read() {
  return mgos_onewire_read((struct mgos_onewire *) ow_);
}

void OneWire::read_bytes(uint8_t *buf, uint16_t count) {
  mgos_onewire_read_bytes(ow_, buf, count);
}

void OneWire::select(const uint8_t rom[8]) {
  mgos_onewire_select(ow_, rom);
}

void OneWire::skip() {
  mgos_onewire_skip(ow_);
}

void OneWire::depower() {
}

void OneWire::reset_search() {
  mgos_onewire_search_clean(ow_);
}

void OneWire::target_search(uint8_t family_code) {
  mgos_onewire_target_setup(ow_, family_code);
}

uint8_t OneWire::search(uint8_t *newAddr, bool search_mode /* = true */) {
  return mgos_onewire_next(ow_, newAddr, !search_mode);
}

uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len) {
  return mgos_onewire_crc8(addr, len);
}
