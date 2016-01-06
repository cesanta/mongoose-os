#include "esp_flasher_client.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QObject>

#include "serial.h"
#include "slip.h"
#include "status_qt.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qInfo qWarning
#endif

namespace {

const quint32 flashBlockSize = 64 * 1024;
// These are rather conservative estimates. Used in timeout calculations.
const quint32 flashBlockReadWriteTimeMs = 250;
const quint32 flashBlockEraseTimeMs = 900;
const quint32 flashEraseMinTimeoutMs = 5000;

const quint32 flashReadBlockSize = 1024;

// Must be in sync with stub_flasher.c
enum stub_cmd {
  CMD_FLASH_ERASE = 0,
  CMD_FLASH_WRITE = 1,
  CMD_FLASH_READ = 2,
  CMD_FLASH_DIGEST = 3,
  CMD_FLASH_READ_CHIP_ID = 4,
  CMD_REBOOT = 5,
};

QByteArray cmdByte(enum stub_cmd cmd) {
  QByteArray result;
  result.append(quint8(cmd));
  return result;
}

}  // namespace

ESPFlasherClient::ESPFlasherClient(ESPROMClient *rom) : rom_(rom) {
}

ESPFlasherClient::~ESPFlasherClient() {
  disconnect();
}

util::Status ESPFlasherClient::connect(qint32 baudRate) {
  const QString prefix = tr("ESPFlasherClient::connect(%1): ").arg(baudRate);
  qDebug() << prefix;
  if (!rom_->connected()) {
    return util::Status(util::error::FAILED_PRECONDITION,
                        "ESPROMClient not connected");
  }

  if (baudRate == rom_->data_port()->baudRate()) baudRate = 0;  // Don't change

  QFile f(":/esp8266/stub_flasher.json");
  if (!f.open(QIODevice::ReadOnly)) {
    return util::Status(util::error::UNAVAILABLE, "Failed to open stub");
  }
  util::Status st = rom_->runStub(f.readAll(), {quint32(baudRate)});
  if (!st.ok()) return QSP(prefix + "runStub failed", st);

  if (baudRate > 0) {
    oldBaudRate_ = rom_->data_port()->baudRate();
    st = setSpeed(rom_->data_port(), baudRate);
    if (!st.ok()) return QSP(prefix + "failed to set baud rate", st);
  }

  auto res = SLIP::recv(rom_->data_port());
  if (!res.ok()) return QSP(prefix + "failed to read hello", res.status());

  const QString greeting = QString::fromLatin1(res.ValueOrDie());

  if (greeting != "OHAI") {
    return QS(util::error::INTERNAL,
              prefix + tr("unexpected greeting: %1").arg(greeting));
  }

  qInfo() << "Connected to flasher";

  return util::Status::OK;
}

util::Status ESPFlasherClient::disconnect() {
  if (oldBaudRate_ > 0) {
    util::Status st = setSpeed(rom_->data_port(), oldBaudRate_);
    if (st.ok()) oldBaudRate_ = 0;
    return st;
  }
  return util::Status::OK;
}

util::Status ESPFlasherClient::erase(quint32 addr, quint32 size) {
  const QString prefix =
      tr("ESPFlasherClient::erase(0x%1, %2): ").arg(addr, 0, 16).arg(size);
  qDebug() << prefix;
  util::Status st = SLIP::send(rom_->data_port(), cmdByte(CMD_FLASH_ERASE));
  if (!st.ok()) return QSP(prefix + "command write failed", st);
  QByteArray args;
  QDataStream s(&args, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << addr << size;
  st = SLIP::send(rom_->data_port(), args);
  if (!st.ok()) return QSP(prefix + "arg write failed", st);
  const int timeoutMs =
      std::max(flashEraseMinTimeoutMs,
               flashBlockEraseTimeMs * (size / flashBlockSize + 1));
  auto res = SLIP::recv(rom_->data_port(), timeoutMs);
  if (!res.ok()) return QSP(prefix + "failed to read response", res.status());
  return util::Status::OK;
}

util::Status ESPFlasherClient::write(quint32 addr, QByteArray data,
                                     bool erase) {
  const QString prefix = tr("ESPFlasherClient::write(0x%1, %2, %3): ")
                             .arg(addr, 0, 16)
                             .arg(data.length())
                             .arg(erase);
  qDebug() << prefix;
  util::Status st = SLIP::send(rom_->data_port(), cmdByte(CMD_FLASH_WRITE));
  if (!st.ok()) {
    return QSP(prefix + "command write failed", st);
  }
  QByteArray args;
  QDataStream s(&args, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << addr << quint32(data.length()) << quint32(erase);
  st = SLIP::send(rom_->data_port(), args);
  if (!st.ok()) return QSP(prefix + "arg write failed", st);
  quint32 numSent = 0, numWritten = 0;
  while (numWritten < quint32(data.length())) {
    int timeoutMs = 200;
    if (numSent == 0 && erase) {
      timeoutMs = std::max(
          flashEraseMinTimeoutMs,
          flashBlockEraseTimeMs * (data.length() / flashBlockSize + 1));
    }
    auto res = SLIP::recv(rom_->data_port(), timeoutMs);
    if (!res.ok()) {
      return QSP(prefix + tr("failed to read response @ %1").arg(numWritten),
                 res.status());
    }
    QByteArray respBytes = res.ValueOrDie();
    if (respBytes.length() == 1) {
      return QS(util::error::UNAVAILABLE,
                prefix +
                    tr("failed to write, code: %1")
                        .arg(QString::fromLatin1(respBytes.toHex())));
    }
    if (respBytes.length() != 4) {
      return QS(
          util::error::INTERNAL,
          prefix + tr("expected 4 bytes, got %1").arg(respBytes.length()));
    }
    QDataStream s(respBytes);
    s.setByteOrder(QDataStream::LittleEndian);
    s >> numWritten;
    emit progress(numWritten);
    while (numSent - numWritten <= 3072 && numSent < quint32(data.length())) {
      quint32 toSend = data.length();
      if (toSend > 1024) toSend = 1024;
      qint64 ns = rom_->data_port()->write(data.mid(numSent, toSend));
      if (ns < 0) {
        return QSP(prefix + tr("failed to write @ %1").arg(numSent), st);
      }
      numSent += ns;
    }
  }
  auto hres = SLIP::recv(rom_->data_port());
  if (!hres.ok()) {
    return QSP(prefix + "digest read failed", hres.status());
  }
  const QByteArray &expHash = hres.ValueOrDie();
  const QByteArray &hash =
      QCryptographicHash::hash(data, QCryptographicHash::Md5);
  if (hash != expHash) {
    return QS(util::error::DATA_LOSS,
              prefix +
                  tr("hash mismatch: expected %3, got %4")
                      .arg(QString::fromLatin1(expHash.toHex()))
                      .arg(QString::fromLatin1(hash.toHex())));
  }
  auto res = SLIP::recv(rom_->data_port());
  if (!res.ok()) {
    return QSP(prefix + tr("failed to read response @ %1").arg(numWritten),
               res.status());
  }
  QByteArray respBytes = res.ValueOrDie();
  if (respBytes.length() != 1) {
    return QS(util::error::INTERNAL,
              prefix + tr("expected 1 byte, got %1").arg(respBytes.length()));
  }
  if (respBytes[0] != '\x00') {
    return QS(util::error::UNAVAILABLE,
              prefix +
                  tr("bad final response, %1")
                      .arg(QString::fromLatin1(respBytes.toHex())));
  }

  return util::Status::OK;
}

util::StatusOr<QByteArray> ESPFlasherClient::read(quint32 addr, quint32 size) {
  const QString prefix =
      tr("ESPFlasherClient::read(0x%1, %2): ").arg(addr, 0, 16).arg(size);
  qDebug() << prefix;
  util::Status st = SLIP::send(rom_->data_port(), cmdByte(CMD_FLASH_READ));
  if (!st.ok()) return QSP(prefix + "command write failed", st);
  QByteArray args;
  QDataStream s(&args, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << addr << size << flashReadBlockSize;
  st = SLIP::send(rom_->data_port(), args);
  if (!st.ok()) return QSP(prefix + "arg write failed", st);
  QByteArray data;
  while (quint32(data.length()) < size) {
    auto bres = SLIP::recv(rom_->data_port());
    if (!bres.ok()) {
      return QSP(prefix + tr("data read failed @ %3").arg(data.length()),
                 bres.status());
    }
    data.append(bres.ValueOrDie());
    emit progress(data.length());
  }
  if (quint32(data.length()) > size) {
    return QS(
        util::error::INTERNAL,
        prefix + tr("expected %3 bytes, got %4").arg(size).arg(data.length()));
  }
  auto hres = SLIP::recv(rom_->data_port());
  if (!hres.ok()) {
    return QSP(prefix + "digest read failed", hres.status());
  }
  const QByteArray &hash = hres.ValueOrDie();
  const QByteArray &expHash =
      QCryptographicHash::hash(data, QCryptographicHash::Md5);
  if (hash != expHash) {
    return QS(util::error::DATA_LOSS,
              prefix +
                  tr("hash mismatch: expected %3, got %4")
                      .arg(QString::fromLatin1(expHash.toHex()))
                      .arg(QString::fromLatin1(hash.toHex())));
  }
  auto sres = SLIP::recv(rom_->data_port());
  if (!sres.ok()) {
    return QSP(prefix + tr("failed to read status"), sres.status());
  }
  /* We don't verify the status. Hash matched, so - whatever. */
  return data;
}

util::StatusOr<ESPFlasherClient::DigestResult> ESPFlasherClient::digest(
    quint32 addr, quint32 size, quint32 digestBlockSize) {
  const QString prefix = tr("ESPFlasherClient::digest(0x%1, %2, %3): ")
                             .arg(addr, 0, 16)
                             .arg(size)
                             .arg(digestBlockSize);
  qDebug() << prefix;
  util::Status st = SLIP::send(rom_->data_port(), cmdByte(CMD_FLASH_DIGEST));
  if (!st.ok()) return QSP(prefix + "command write failed", st);
  QByteArray args;
  QDataStream s(&args, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << addr << size << digestBlockSize;
  st = SLIP::send(rom_->data_port(), args);
  if (!st.ok()) return QSP(prefix + "arg write failed", st);
  DigestResult dres;
  while (true) {
    int timeoutMs = flashBlockReadWriteTimeMs *
                    (digestBlockSize > 0 ? 10 : (size / flashBlockSize + 1));
    auto res = SLIP::recv(rom_->data_port(), timeoutMs);
    if (!res.ok()) return QSP(prefix + "read failed", res.status());
    QByteArray r = res.ValueOrDie();
    switch (r.length()) {
      case 16: {
        if (dres.digest.length() > 0) {
          dres.blockDigests.push_back(dres.digest);
        }
        dres.digest = r;
        break;
      }
      case 1: {
        return dres;
      }
      default: {
        return QS(util::error::INTERNAL,
                  tr("unexpected response length: %1").arg(r.length()));
      }
    }
  }
  // Not reached.
}

util::StatusOr<quint32> ESPFlasherClient::getFlashChipID() {
  const QString prefix = tr("ESPFlasherClient::getFlashChipID(): ");
  qDebug() << prefix;
  util::Status st =
      SLIP::send(rom_->data_port(), cmdByte(CMD_FLASH_READ_CHIP_ID));
  if (!st.ok()) return QSP(prefix + "command write failed", st);
  auto res = SLIP::recv(rom_->data_port(), 1000);
  if (!res.ok()) return QSP(prefix + "failed to read result", res.status());
  quint32 chipID = 0;
  QByteArray respBytes = res.ValueOrDie();
  if (respBytes.length() != 4)
    return QS(util::error::INTERNAL,
              prefix + tr("invalid result length: %1").arg(respBytes.length()));
  QDataStream s(respBytes);
  s.setByteOrder(QDataStream::BigEndian);  // BigEndian to preserve byte order.
  s >> chipID;
  if (chipID == 0)
    return QS(util::error::INTERNAL, prefix + "failed to get chip id");
  res = SLIP::recv(rom_->data_port());
  if (!res.ok()) return QSP(prefix + "failed to read status", res.status());
  return chipID;
}

util::Status ESPFlasherClient::reboot() {
  qDebug() << "ESPFlasherClient::reboot()";
  util::Status st = SLIP::send(rom_->data_port(), cmdByte(CMD_REBOOT));
  if (!st.ok()) return QSP(tr("reboot(): command write failed"), st);
  auto res = SLIP::recv(rom_->data_port(), 15000);
  if (!st.ok()) return QSP(tr("reboot(): failed to read response"), st);
  return util::Status::OK;
}
