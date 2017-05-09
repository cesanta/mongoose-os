/*
 * Arduino 1-Wire API wrapper for compatibility
 *
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "OneWire.h"
#include "fw/src/mgos_onewire.h"

OneWire::OneWire(uint8_t pin) {
  ow = mgos_onewire_init(pin);
}

OneWire::~OneWire() {
  mgos_onewire_close((struct mgos_onewire *) ow);
}

uint8_t OneWire::reset(void) {
  return mgos_onewire_reset((struct mgos_onewire *) ow);
}

void OneWire::write_bit(uint8_t v) {
  mgos_onewire_write_bit((struct mgos_onewire *) ow, v);
}

uint8_t OneWire::read_bit(void) {
  return mgos_onewire_read_bit((struct mgos_onewire *) ow);
}

void OneWire::write(uint8_t v, uint8_t power /* = 0 */) {
  (void) power;
  mgos_onewire_write((struct mgos_onewire *) ow, v);
}

void OneWire::write_bytes(const uint8_t *buf, uint16_t count,
                          bool power /* = 0 */) {
  (void) power;
  mgos_onewire_write_bytes((struct mgos_onewire *) ow, buf, count);
}

uint8_t OneWire::read() {
  return mgos_onewire_read((struct mgos_onewire *) ow);
}

void OneWire::read_bytes(uint8_t *buf, uint16_t count) {
  mgos_onewire_read_bytes((struct mgos_onewire *) ow, buf, count);
}

void OneWire::select(const uint8_t rom[8]) {
  mgos_onewire_select((struct mgos_onewire *) ow, rom);
}

void OneWire::skip() {
  mgos_onewire_skip((struct mgos_onewire *) ow);
}

void OneWire::depower() {
}

void OneWire::reset_search() {
  mgos_onewire_search_clean((struct mgos_onewire *) ow);
}

void OneWire::target_search(uint8_t family_code) {
  mgos_onewire_target_setup((struct mgos_onewire *) ow, family_code);
}

uint8_t OneWire::search(uint8_t *newAddr, bool search_mode /* = true */) {
  return mgos_onewire_next((struct mgos_onewire *) ow, newAddr, !search_mode);
}

uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len) {
  return mgos_onewire_crc8(addr, len);
}
