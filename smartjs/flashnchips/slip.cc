#include "slip.h"

#include <QDebug>

#include "status_qt.h"

namespace SLIP {

// https://tools.ietf.org/html/rfc1055
const unsigned char SLIPFrameDelimiter = 0xC0;
const unsigned char SLIPEscape = 0xDB;
const unsigned char SLIPEscapeFrameDelimiter = 0xDC;
const unsigned char SLIPEscapeEscape = 0xDD;

util::Status send(QSerialPort *port, const QByteArray &data, int timeoutMs) {
  const QString prefix = QString("SLIP::send(%1, %2, %3):")
                             .arg(port->portName())
                             .arg(data.length())
                             .arg(timeoutMs);
  qDebug() << prefix << "=>" << data.toHex();
  bool ok = port->putChar(SLIPFrameDelimiter);
  for (int i = 0; ok && i < data.length(); i++) {
    switch ((unsigned char) data[i]) {
      case SLIPFrameDelimiter:
        ok = ok && port->putChar(SLIPEscape);
        ok = ok && port->putChar(SLIPEscapeFrameDelimiter);
        break;
      case SLIPEscape:
        ok = ok && port->putChar(SLIPEscape);
        ok = ok && port->putChar(SLIPEscapeEscape);
        break;
      default:
        ok = ok && port->putChar(data[i]);
        break;
    }
  }
  ok = ok && port->putChar(SLIPFrameDelimiter);
  ok = ok && port->waitForBytesWritten(timeoutMs);
  if (!ok) {
    return QS(util::error::UNAVAILABLE, prefix + " " + port->errorString());
  }
  return util::Status::OK;
}

util::StatusOr<QByteArray> recv(QSerialPort *port, int timeoutMs) {
  const QString prefix =
      QString("SLIP::recv(%1, %2): ").arg(port->portName()).arg(timeoutMs);
  QByteArray ret;
  char c = 0;
  // Skip everything before the frame start.
  do {
    if (port->bytesAvailable() == 0 && !port->waitForReadyRead(timeoutMs)) {
      return QS(util::error::UNAVAILABLE,
                prefix + "no data 1: " + port->errorString());
    }
    if (!port->getChar(&c)) {
      return QS(util::error::UNAVAILABLE,
                prefix + "failed to read prefix: " + port->errorString());
    }
  } while ((unsigned char) c != SLIPFrameDelimiter);
  for (;;) {
    if (port->bytesAvailable() == 0 && !port->waitForReadyRead(timeoutMs)) {
      return QS(util::error::UNAVAILABLE,
                prefix + "no data 2: " + port->errorString());
    }
    if (!port->getChar(&c)) {
      return QS(util::error::UNAVAILABLE, prefix + "failed to read next char");
    }
    switch ((unsigned char) c) {
      case SLIPFrameDelimiter:
        // End of frame.
        qDebug() << "<=" << ret.toHex();
        return ret;
      case SLIPEscape:
        if (port->bytesAvailable() == 0 && !port->waitForReadyRead(timeoutMs)) {
          return QS(util::error::UNAVAILABLE,
                    prefix + "no data 3: " + port->errorString());
        }
        if (!port->getChar(&c)) {
          return QS(util::error::UNAVAILABLE,
                    prefix + "failed to read next char");
        }
        switch ((unsigned char) c) {
          case SLIPEscapeFrameDelimiter:
            ret.append(SLIPFrameDelimiter);
            break;
          case SLIPEscapeEscape:
            ret.append(SLIPEscape);
            break;
          default:
            return QS(util::error::UNAVAILABLE,
                      prefix + "invalid escape sequence: " + int(c));
        }
        break;
      default:
        ret.append(c);
        break;
    }
  }
}

}  // namespace SLIP
