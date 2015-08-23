#include "serial.h"

#include <memory>
#ifdef Q_OS_OSX
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#endif

#include <QCoreApplication>
#include <QSerialPort>

#include <common/util/error_codes.h>
#include <common/util/status.h>

util::StatusOr<QSerialPort*> connectSerial(const QSerialPortInfo& port,
                                           int speed) {
  std::unique_ptr<QSerialPort> s(new QSerialPort(port));
  if (!s->setParity(QSerialPort::NoParity)) {
    return util::Status(
        util::error::INTERNAL,
        QCoreApplication::translate("connectSerial", "Failed to disable parity")
            .toStdString());
  }
  if (!s->setFlowControl(QSerialPort::NoFlowControl)) {
    return util::Status(
        util::error::INTERNAL,
        QCoreApplication::translate(
            "connectSerial", "Failed to disable flow control").toStdString());
  }
  if (!s->open(QIODevice::ReadWrite)) {
    return util::Status(util::error::INTERNAL,
                        QCoreApplication::translate(
                            "connectSerial", "Failed to open").toStdString());
  }
  if (!s->setBaudRate(speed)) {
    return util::Status(
        util::error::INTERNAL,
        QCoreApplication::translate("connectSerial", "Failed to set baud rate")
            .toStdString());
  }
#ifdef Q_OS_OSX
  if (ioctl(s->handle(), IOSSIOSPEED, &speed) < 0) {
    return util::Status(
        util::error::INTERNAL,
        QCoreApplication::translate("connectSerial",
                                    "Failed to set baud rate with ioctl")
            .toStdString());
  }
#endif
  return s.release();
}
