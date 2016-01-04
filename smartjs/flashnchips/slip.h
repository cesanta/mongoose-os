#ifndef SLIP_H
#define SLIP_H

#include <QSerialPort>
#include <QByteArray>

#include <common/util/statusor.h>

namespace SLIP {

util::StatusOr<QByteArray> recv(QSerialPort *in, int timeout = 500);
util::Status send(QSerialPort *out, const QByteArray &bytes,
                  int timeoutMs = 500);

}  // namespace SLIP

#endif /* SLIP_H */
