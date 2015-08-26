#ifndef HAL_H
#define HAL_H

#include <memory>

#include <common/util/status.h>

#include "flasher.h"

class QSerialPort;
class QSerialPortInfo;

class HAL {
 public:
  virtual ~HAL(){};
  virtual util::Status probe(const QSerialPortInfo&) const = 0;
  virtual std::unique_ptr<Flasher> flasher() const = 0;
  virtual std::string name() const = 0;
  virtual util::Status reboot(QSerialPort*) const = 0;
};

#endif  // HAL_H
