/*
 * Arduino Wire library API wrapper for compatibility
 *
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "Wire.h"

TwoWire Wire;

TwoWire::TwoWire()
    : i2c(nullptr),
      addr(0),
      n_bytes_avail(0),
      n_bytes_to_send(0),
      pbyte_to_recv(nullptr),
      pbyte_to_send(nullptr) {
  on_receive_cb = nullptr;
  on_request_cb = nullptr;
}

TwoWire::~TwoWire() {
}

void TwoWire::begin(void) {
  if (i2c == nullptr) i2c = mgos_i2c_get_global();
  if (i2c == nullptr) return;

  mgos_i2c_stop(i2c);

  pbyte_to_recv = recv_buf;
  pbyte_to_send = send_buf;
  n_bytes_avail = n_bytes_to_send = 0;
}

void TwoWire::begin(uint8_t address) {
  addr = address;
  begin();
}

void TwoWire::begin(int address) {
  begin((uint8_t) address);
}

void TwoWire::end(void) {
  if (i2c == nullptr) return;
  mgos_i2c_stop(i2c);
  mgos_i2c_close(i2c);
  i2c = nullptr;
}

void TwoWire::setClock(uint32_t clock) {
  (void) clock;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity,
                             uint32_t iaddress, uint8_t isize,
                             uint8_t sendStop) {
  (void) iaddress;
  (void) isize;

  if (i2c == nullptr) return 0;
  if (quantity > BUF_SIZE) quantity = BUF_SIZE;

  bool res = mgos_i2c_read(i2c, address, recv_buf, quantity, sendStop);
  pbyte_to_recv = recv_buf;
  n_bytes_avail = res ? quantity : 0;

  return n_bytes_avail;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity,
                             uint8_t sendStop) {
  return requestFrom((uint8_t) address, (uint8_t) quantity, (uint32_t) 0,
                     (uint8_t) 0, (uint8_t) sendStop);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity) {
  return requestFrom((uint8_t) address, (uint8_t) quantity, (uint8_t) true);
}

uint8_t TwoWire::requestFrom(int address, int quantity) {
  return requestFrom((uint8_t) address, (uint8_t) quantity, (uint8_t) true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop) {
  return requestFrom((uint8_t) address, (uint8_t) quantity, (uint8_t) sendStop);
}

void TwoWire::beginTransmission(uint8_t address) {
  addr = address;
  pbyte_to_send = send_buf;
  n_bytes_to_send = 0;
}

void TwoWire::beginTransmission(int address) {
  beginTransmission((uint8_t) address);
}

uint8_t TwoWire::endTransmission(uint8_t sendStop) {
  if (i2c == nullptr) return 4;

  bool res = mgos_i2c_write(i2c, addr, send_buf, n_bytes_to_send, sendStop);

  pbyte_to_send = send_buf;
  n_bytes_to_send = 0;

  return res ? 0 : 4;
}

uint8_t TwoWire::endTransmission(void) {
  return endTransmission(true);
}

size_t TwoWire::write(uint8_t data) {
  if (n_bytes_to_send >= BUF_SIZE) return 0;
  *pbyte_to_send++ = data;
  n_bytes_to_send++;
  return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity) {
  for (size_t i = 0; i < quantity; i++) write(data[i]);
  return quantity;
}

int TwoWire::available(void) {
  return recv_buf + n_bytes_avail - pbyte_to_recv;
}

int TwoWire::read(void) {
  return (pbyte_to_recv < recv_buf + n_bytes_avail) ? *pbyte_to_recv++ : -1;
}

int TwoWire::peek(void) {
  return (pbyte_to_recv < recv_buf + n_bytes_avail) ? *pbyte_to_recv : -1;
}

void TwoWire::flush(void) {
}

void TwoWire::onReceiveService(uint8_t *inBytes, int numBytes) {
  if (on_receive_cb == nullptr) return;
  if (pbyte_to_recv < recv_buf + n_bytes_avail) return;

  for (int i = 0; i < numBytes; i++) recv_buf[i] = inBytes[i];

  pbyte_to_recv = recv_buf;
  n_bytes_avail = numBytes;

  on_receive_cb(numBytes);
}

void TwoWire::onRequestService(void) {
  if (on_request_cb == nullptr) return;

  pbyte_to_send = send_buf;
  n_bytes_to_send = 0;

  on_request_cb();
}

void TwoWire::onReceive(void (*function)(int)) {
  on_receive_cb = function;
}

void TwoWire::onRequest(void (*function)(void)) {
  on_request_cb = function;
}
