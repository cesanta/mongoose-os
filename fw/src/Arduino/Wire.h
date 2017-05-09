#ifndef CS_FW_SRC_ARDUINO_WIRE_H_
#define CS_FW_SRC_ARDUINO_WIRE_H_

#include "fw/src/mgos_features.h"

#include <stdint.h>
#include "Arduino.h"
#include "fw/src/mgos_i2c.h"

struct mgos_i2c {
  int sda_gpio;
  int scl_gpio;
  bool started;
};

class TwoWire {
 private:
  enum { BUF_SIZE = 32 };

  struct mgos_i2c *i2c;
  uint8_t addr, n_bytes_avail, n_bytes_to_send;
  uint8_t *pbyte_to_recv, *recv_buf;
  uint8_t *pbyte_to_send, *send_buf;

  void (*on_request_cb)(void);
  void (*on_receive_cb)(int);
  void onRequestService(void);
  void onReceiveService(uint8_t *, int);

 public:
  TwoWire();
  ~TwoWire();
  void begin();
  void begin(uint8_t);
  void begin(int);
  void end();
  void setClock(uint32_t);
  void beginTransmission(uint8_t);
  void beginTransmission(int);
  uint8_t endTransmission(void);
  uint8_t endTransmission(uint8_t);
  uint8_t requestFrom(uint8_t, uint8_t);
  uint8_t requestFrom(uint8_t, uint8_t, uint8_t);
  uint8_t requestFrom(uint8_t, uint8_t, uint32_t, uint8_t, uint8_t);
  uint8_t requestFrom(int, int);
  uint8_t requestFrom(int, int, int);
  size_t write(uint8_t);
  size_t write(const uint8_t *, size_t);
  int available(void);
  int read(void);
  int peek(void);
  void flush(void);
  void onReceive(void (*)(int));
  void onRequest(void (*)(void));

  size_t write(unsigned long n) {
    return write((uint8_t) n);
  }
  size_t write(long n) {
    return write((uint8_t) n);
  }
  size_t write(unsigned int n) {
    return write((uint8_t) n);
  }
  size_t write(int n) {
    return write((uint8_t) n);
  }
};

#endif /* CS_FW_SRC_ARDUINO_WIRE_H_ */
