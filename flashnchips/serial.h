#ifndef SERIAL_H
#define SERIAL_H

#include <QSerialPortInfo>

#include <common/util/statusor.h>

class QSerialPort;

util::StatusOr<QSerialPort*> connectSerial(const QSerialPortInfo& port,
                                           int speed = 115200);

#endif  // SERIAL_H
