#include "esp8266.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QtDebug>
#include <QIODevice>
#include <QMutex>
#include <QMutexLocker>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTextStream>
#include <QThread>

#include <common/util/error_codes.h>
#include <common/util/statusor.h>

#include "fs.h"
#include "serial.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qInfo qWarning
#endif

// Code in this file (namely, rebootIntoBootloader function) assumes the same
// wiring as esptool.py:
//   RTS - CH_PD or RESET pin
//   DTR - GPIO0 pin

namespace ESP8266 {

const char kFlashParamsOption[] = "esp8266-flash-params";
const char kDisableEraseWorkaroundOption[] = "esp8266-disable-erase-workaround";
const char kSkipReadingFlashParamsOption[] =
    "esp8266-skip-reading-flash-params";
const char kFlashingDataPort[] = "esp8266-flashing-data-port";
const char kInvertDTRRTS[] = "esp8266-invert-dtr-rts";

namespace {

const ulong writeBlockSize = 0x400;
const ulong flashBlockSize = 4096;
const char fwFileGlob[] = "0x*.bin";
const ulong idBlockOffset = 0x10000;
const ulong idBlockSize = flashBlockSize;
const ulong spiffsBlockOffset = 0x6d000;
const ulong spiffsBlockSize = 0x10000;

// Copy-pasted from
// https://github.com/themadinventor/esptool/blob/e96336f6561109e67afe03c0695d1e5b0de15da6/esptool.py
// Copyright (C) 2014 Fredrik Ahlberg
// License: GPLv2
// Must be prefixed with 3 32-bit arguments: offset, blocklen, blockcount.
// Updated to reboot after reading.
const char ESPReadFlashStub[] =
    "\x80\x3c\x00\x40"  // data: send_packet
    "\x1c\x4b\x00\x40"  // data: SPIRead
    "\x80\x00\x00\x40"  // data: ResetVector
    "\x00\x80\xfe\x3f"  // data: buffer
    "\xc1\xfb\xff"      //       l32r    a12, $blockcount
    "\xd1\xf8\xff"      //       l32r    a13, $offset
    "\x2d\x0d"          // loop: mov.n   a2, a13
    "\x31\xfd\xff"      //       l32r    a3, $buffer
    "\x41\xf7\xff"      //       l32r    a4, $blocklen
    "\x4a\xdd"          //       add.n   a13, a13, a4
    "\x51\xf9\xff"      //       l32r    a5, $SPIRead
    "\xc0\x05\x00"      //       callx0  a5
    "\x21\xf9\xff"      //       l32r    a2, $buffer
    "\x31\xf3\xff"      //       l32r    a3, $blocklen
    "\x41\xf5\xff"      //       l32r    a4, $send_packet
    "\xc0\x04\x00"      //       callx0  a4
    "\x0b\xcc"          //       addi.n  a12, a12, -1
    "\x56\xec\xfd"      //       bnez    a12, loop
    "\x61\xf4\xff"      //       l32r    a6, $ResetVector
    "\xa0\x06\x00"      //       jx      a6
    "\x00\x00\x00"      //       // padding
    ;

// ESP8266 bootloader uses SLIP frame format for communication.
// https://tools.ietf.org/html/rfc1055
const unsigned char SLIPFrameDelimiter = 0xC0;
const unsigned char SLIPEscape = 0xDB;
const unsigned char SLIPEscapeFrameDelimiter = 0xDC;
const unsigned char SLIPEscapeEscape = 0xDD;

qint64 SLIP_write(QSerialPort* out, const QByteArray& bytes) {
  // XXX: errors are ignored.
  qDebug() << "Writing bytes" << out->portName() << ":" << bytes.toHex();
  out->putChar(SLIPFrameDelimiter);
  for (int i = 0; i < bytes.length(); i++) {
    switch ((unsigned char) bytes[i]) {
      case SLIPFrameDelimiter:
        out->putChar(SLIPEscape);
        out->putChar(SLIPEscapeFrameDelimiter);
        break;
      case SLIPEscape:
        out->putChar(SLIPEscape);
        out->putChar(SLIPEscapeEscape);
        break;
      default:
        out->putChar(bytes[i]);
        break;
    }
  }
  out->putChar(SLIPFrameDelimiter);
  if (!out->waitForBytesWritten(200)) {
    qDebug() << "Error: " << out->errorString();
  }
  return bytes.length();
}

QByteArray SLIP_read(QSerialPort* in, int readTimeout = 200) {
  QByteArray ret;
  char c = 0;
  // Skip everything before the frame start.
  do {
    if (in->bytesAvailable() == 0 && !in->waitForReadyRead(readTimeout)) {
      qDebug() << "No data: " << in->errorString();
      return ret;
    }
    if (!in->getChar(&c)) {
      qDebug() << "Failed to read prefix: " << in->errorString();
      return ret;
    }
  } while ((unsigned char) c != SLIPFrameDelimiter);
  for (;;) {
    if (in->bytesAvailable() == 0 && !in->waitForReadyRead(readTimeout)) {
      qDebug() << "No data: " << in->errorString();
      return ret;
    }
    if (!in->getChar(&c)) {
      qDebug() << "Failed to read next char";
      return ret;
    }
    switch ((unsigned char) c) {
      case SLIPFrameDelimiter:
        // End of frame.
        qDebug() << "Read bytes" << in->portName() << ":" << ret.toHex();
        return ret;
      case SLIPEscape:
        if (in->bytesAvailable() == 0 && !in->waitForReadyRead(readTimeout)) {
          qDebug() << "No data: " << in->errorString();
          return ret;
        }
        if (!in->getChar(&c)) {
          qDebug() << "Failed to read next char";
          return ret;
        }
        switch ((unsigned char) c) {
          case SLIPEscapeFrameDelimiter:
            ret.append(SLIPFrameDelimiter);
            break;
          case SLIPEscapeEscape:
            ret.append(SLIPEscape);
            break;
          default:
            qDebug() << "Invalid escape sequence: " << int(c);
            return ret;
        }
        break;
      default:
        ret.append(c);
        break;
    }
  }
}

quint8 checksum(const QByteArray& data) {
  quint8 r = 0xEF;
  for (int i = 0; i < data.length(); i++) {
    r ^= data[i];
  }
  return r;
}

void writeCommand(QSerialPort* out, quint8 cmd, const QByteArray& payload,
                  quint8 csum = 0) {
  QByteArray frame;
  QDataStream s(&frame, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << quint8(0) << cmd << quint16(payload.length());
  s << quint32(csum);  // Yes, it is indeed padded with 3 zero bytes.
  frame.append(payload);
  SLIP_write(out, frame);
}

struct Response {
  quint8 command = 0xff;
  QByteArray value;
  QByteArray body;
  quint8 status = 0;
  quint8 lastError = 0;
  bool valid = false;

  bool ok() const {
    return valid && status == 0;
  }

  QString error() const {
    if (!valid) {
      return "invalid response";
    }
    if (status != 0 || lastError != 0) {
      return QString("status: %1 %2").arg(status).arg(lastError);
    }
    return "";
  }
};

Response readResponse(QSerialPort* in, int timeout = 200) {
  Response ret;
  ret.valid = false;
  QByteArray resp = SLIP_read(in, timeout);
  if (resp.length() < 10) {
    qDebug() << "Incomplete response: " << resp.toHex();
    return ret;
  }
  QDataStream s(resp);
  s.setByteOrder(QDataStream::LittleEndian);
  quint8 direction;
  s >> direction;
  if (direction != 1) {
    qDebug() << "Invalid direction (first byte) in response: " << resp.toHex();
    return ret;
  }

  quint16 size;
  s >> ret.command >> size;

  ret.value.resize(4);
  char* buf = ret.value.data();
  s.readRawData(buf, 4);

  ret.body.resize(size);
  buf = ret.body.data();
  s.readRawData(buf, size);
  if (ret.body.length() == 2) {
    ret.status = ret.body[0];
    ret.lastError = ret.body[1];
  }

  ret.valid = true;
  return ret;
}

QByteArray read_register(QSerialPort* serial, quint32 addr) {
  QByteArray payload;
  QDataStream s(&payload, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << addr;
  writeCommand(serial, 0x0A, payload);
  auto resp = readResponse(serial);
  if (!resp.valid) {
    qDebug() << "Invalid response to command " << 0x0A;
    return QByteArray();
  }
  if (resp.command != 0x0A) {
    qDebug() << "Response to unexpected command: " << resp.command;
    return QByteArray();
  }
  if (resp.status != 0) {
    qDebug() << "Bad response status: " << resp.status;
    return QByteArray();
  }
  return resp.value;
}

QByteArray read_MAC(QSerialPort* serial) {
  QByteArray ret;
  auto mac1 = read_register(serial, 0x3ff00050);
  auto mac2 = read_register(serial, 0x3ff00054);
  if (mac1.length() != 4 || mac2.length() != 4) {
    return ret;
  }
  QDataStream s(&ret, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  switch (mac2[2]) {
    case 0:
      s << quint8(0x18) << quint8(0xFE) << quint8(0x34);
      break;
    case 1:
      s << quint8(0xAC) << quint8(0xD0) << quint8(0x74);
      break;
    default:
      qDebug() << "Unknown OUI";
      return ret;
  }
  s << quint8(mac2[1]) << quint8(mac2[0]) << quint8(mac1[3]);
  return ret;
}

bool sync(QSerialPort* serial) {
  QByteArray payload("\x07\x07\x12\x20");
  payload.append(QByteArray("\x55").repeated(32));
  writeCommand(serial, 0x08, payload);
  for (int i = 0; i < 8; i++) {
    auto resp = readResponse(serial);
    if (!resp.valid) {
      return false;
    }
  }
  return true;
}

bool trySync(QSerialPort* serial, int attempts) {
  for (; attempts > 0; attempts--) {
    if (sync(serial)) {
      return true;
    }
  }
  return false;
}

bool rebootIntoBootloader(QSerialPort* serial, bool inverted,
                          QSerialPort* data_port = nullptr) {
  serial->setDataTerminalReady(inverted);
  serial->setRequestToSend(!inverted);
  QThread::msleep(50);
  serial->setDataTerminalReady(!inverted);
  serial->setRequestToSend(inverted);
  QThread::msleep(50);
  serial->setDataTerminalReady(inverted);
  return trySync(data_port != nullptr ? data_port : serial, 3);
}

void rebootIntoFirmware(QSerialPort* serial, bool inverted) {
  serial->setDataTerminalReady(inverted);  // pull up GPIO0
  serial->setRequestToSend(!inverted);     // pull down RESET
  QThread::msleep(50);
  serial->setRequestToSend(inverted);  // pull up RESET
}

class FlasherImpl : public Flasher {
  Q_OBJECT
 public:
  FlasherImpl() : id_hostname_("api.cesanta.com") {
  }

  util::Status setOption(const QString& name, const QVariant& value) override {
    if (name == kIdDomainOption) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      id_hostname_ = value.toString();
      return util::Status::OK;
    } else if (name == kOverwriteFSOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      merge_flash_filesystem_ = !value.toBool();
      return util::Status::OK;
    } else if (name == kSkipIdGenerationOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      generate_id_if_none_found_ = !value.toBool();
      return util::Status::OK;
    } else if (name == kFlashParamsOption) {
      if (value.type() == QVariant::String) {
        auto r = flashParamsFromString(value.toString());
        if (!r.ok()) {
          return r.status();
        }
        override_flash_params_ = r.ValueOrDie();
      } else if (value.canConvert<int>()) {
        override_flash_params_ = value.toInt();
      } else {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a number or a string");
      }
      return util::Status::OK;
    } else if (name == kDisableEraseWorkaroundOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      erase_bug_workaround_ = !value.toBool();
      return util::Status::OK;
    } else if (name == kSkipReadingFlashParamsOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      preserve_flash_params_ = !value.toBool();
      return util::Status::OK;
    } else if (name == kFlashingDataPort) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      flashing_port_name_ = value.toString();
      return util::Status::OK;
    } else if (name == kFlashBaudRate) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      bool ok = false;
      flashing_speed_ = value.toString().toInt(&ok, 10);
      if (!ok) {
        flashing_speed_ = 230400;
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a decimal number");
      }
      return util::Status::OK;
    } else if (name == kInvertDTRRTS) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      invert_dtr_rts_ = value.toBool();
      return util::Status::OK;
    } else {
      return util::Status(util::error::INVALID_ARGUMENT, "unknown option");
    }
  }

  util::Status setOptionsFromCommandLine(
      const QCommandLineParser& parser) override {
    util::Status r;

    QStringList boolOpts({kOverwriteFSOption, kSkipIdGenerationOption,
                          kDisableEraseWorkaroundOption,
                          kSkipReadingFlashParamsOption, kInvertDTRRTS});
    QStringList stringOpts({kIdDomainOption, kFlashParamsOption,
                            kFlashingDataPort, kFlashBaudRate});

    for (const auto& opt : boolOpts) {
      auto s = setOption(opt, parser.isSet(opt));
      if (!s.ok()) {
        r = util::Status(
            s.error_code(),
            (opt + ": " + s.error_message().c_str()).toStdString());
      }
    }
    for (const auto& opt : stringOpts) {
      // XXX: currently there's no way to "unset" a string option.
      if (parser.isSet(opt)) {
        auto s = setOption(opt, parser.value(opt));
        if (!s.ok()) {
          r = util::Status(
              s.error_code(),
              (opt + ": " + s.error_message().c_str()).toStdString());
        }
      }
    }
    return r;
  }

  util::Status load(const QString& path) override {
    QMutexLocker lock(&lock_);
    images_.clear();
    QDir dir(path, fwFileGlob, QDir::Name,
             QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    if (!dir.exists()) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          tr("Directory does not exist").toStdString());
    }

    const auto files = dir.entryInfoList();
    if (files.length() == 0) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          tr("Do files to flash").toStdString());
    }
    for (const auto& file : files) {
      qInfo() << "Loading" << file.fileName();
      bool ok = false;
      ulong addr = file.baseName().toULong(&ok, 16);
      if (!ok) {
        return util::Status(
            util::error::INVALID_ARGUMENT,
            tr("%1 is not a valid address").arg(file.baseName()).toStdString());
      }
      QFile f(file.absoluteFilePath());
      if (!f.open(QIODevice::ReadOnly)) {
        return util::Status(
            util::error::ABORTED,
            tr("Failed to open %1").arg(file.absoluteFilePath()).toStdString());
      }
      auto bytes = f.readAll();
      if (bytes.length() != file.size()) {
        images_.clear();
        return util::Status(util::error::UNAVAILABLE,
                            tr("%1 has size %2, but readAll returned %3 bytes")
                                .arg(file.fileName())
                                .arg(file.size())
                                .arg(bytes.length())
                                .toStdString());
      }
      images_[addr] = bytes;
    }
    const QList<ulong> keys = images_.keys();
    for (int i = 0; i < keys.length() - 1; i++) {
      if (keys[i] + images_[keys[i]].length() > keys[i + 1]) {
        return util::Status(util::error::FAILED_PRECONDITION,
                            tr("Images at offsets 0x%1 and 0x%2 overlap.")
                                .arg(keys[i], 0, 16)
                                .arg(keys[i + 1], 0, 16)
                                .toStdString());
      }
    }

    files_.clear();
    if (dir.exists("fs")) {
      QDir files_dir(dir.filePath("fs"), "", QDir::Name,
                     QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
      for (const auto& file : files_dir.entryInfoList()) {
        qInfo() << "Loading" << file.fileName();
        QFile f(file.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) {
          return util::Status(util::error::ABORTED,
                              tr("Failed to open %1")
                                  .arg(file.absoluteFilePath())
                                  .toStdString());
        }
        files_.insert(file.fileName(), f.readAll());
        f.close();
      }
    }

    return util::Status::OK;
  }

  util::Status setPort(QSerialPort* port) override {
    QMutexLocker lock(&lock_);
    port_ = port;
    return util::Status::OK;
  }

  int totalBlocks() const override {
    QMutexLocker lock(&lock_);
    int r = 0;
    for (const auto& bytes : images_.values()) {
      r += bytes.length() / writeBlockSize;
      if (bytes.length() % writeBlockSize != 0) {
        r++;
      }
    }
    return r;
  }

  void run() override {
    QMutexLocker lock(&lock_);

    util::Status st = runLocked();
    if (!st.ok()) {
      emit done(QString::fromStdString(st.error_message()), false);
      return;
    }
    emit done(tr("All done!"), true);
  }

 private:
  util::Status runLocked() {
    QSerialPort* data_port = port_;
    std::unique_ptr<QSerialPort> second_port;  // used to close separate data
                                               // port, if any, when we're done

    if (!flashing_port_name_.isEmpty()) {
      const auto& ports = QSerialPortInfo::availablePorts();
      QSerialPortInfo info;
      bool found = false;
      for (const auto& port : ports) {
        if (port.systemLocation() != flashing_port_name_) {
          continue;
        }
        info = port;
        found = true;
        break;
      }
      if (!found) {
        return util::Status(
            util::error::NOT_FOUND,
            tr("Port %1 not found").arg(flashing_port_name_).toStdString());
      }
      auto serial = connectSerial(info, flashing_speed_);
      if (!serial.ok()) {
        return util::Status(
            util::error::UNKNOWN,
            tr("Failed to open %1: %2")
                .arg(flashing_port_name_)
                .arg(QString::fromStdString(serial.status().ToString()))
                .toStdString());
      }
      second_port.reset(serial.ValueOrDie());
      data_port = second_port.get();
    }

    auto st = setSpeed(data_port, flashing_speed_);
    if (!st.ok()) {
      return st;
    }

    if (!rebootIntoBootloader(port_, invert_dtr_rts_, data_port)) {
      return util::Status(
          util::error::UNKNOWN,
          tr("Failed to talk to bootloader. See <a "
             "href=\"https://github.com/cesanta/smart.js/blob/master/"
             "platforms/esp8266/flashing.md\">wiring instructions</a>.")
              .toStdString());
    }

    int flashParams = -1;
    if (override_flash_params_ >= 0) {
      flashParams = override_flash_params_;
    } else if (preserve_flash_params_) {
      // Here we're trying to read 2 bytes from already flashed firmware and
      // copy them to the image we're about to write. These 2 bytes (bytes 2 and
      // 3, counting from zero) are the paramters of the flash chip needed by
      // ESP8266 SDK code to properly boot the device.
      // TODO(imax): before reading from flash try to check if have the correct
      // params for the flash chip by its ID.
      util::StatusOr<QByteArray> r = readFlashParamsLocked(data_port);
      if (!r.ok()) {
        emit statusMessage(tr("Failed to read flash params: %1")
                               .arg(r.status().error_message().c_str()),
                           true);
        return util::Status(
            util::error::UNKNOWN,
            tr("Failed to read flash params from the existing firmware")
                .toStdString());
      }
      const QByteArray params = r.ValueOrDie();
      if (params.length() == 2) {
        emit statusMessage(
            tr("Current flash params bytes: %1").arg(QString(params.toHex())),
            true);
        flashParams = params[0] << 8 | params[1];
      } else {
        qCritical()
            << "Flash params of unexpected length (want 2 bytes), ignored: "
            << params.toHex();
      }
    }

    if (images_.contains(0) && images_[0].length() >= 4 &&
        images_[0][0] == (char) 0xE9) {
      if (flashParams >= 0) {
        images_[0][2] = (flashParams >> 8) & 0xff;
        images_[0][3] = flashParams & 0xff;
        emit statusMessage(
            tr("Adjusting flash params in the image 0x0000 to %1")
                .arg(QString(images_[0].mid(2, 2).toHex())),
            true);
      }
      flashParams = images_[0][2] << 8 | images_[0][3];
    }

    bool id_generated = false;
    if (generate_id_if_none_found_ && !images_.contains(idBlockOffset)) {
      auto res = findIdLocked(data_port);
      if (res.ok()) {
        if (!res.ValueOrDie()) {
          emit statusMessage(tr("Generating new ID"), true);
          images_[idBlockOffset] = makeIDBlock(id_hostname_);
          id_generated = true;
        } else {
          emit statusMessage(tr("Existing ID found"), true);
        }
      } else {
        emit statusMessage(tr("Failed to read existing ID block: %1")
                               .arg(res.status().ToString().c_str()),
                           true);
        return util::Status(
            util::error::UNKNOWN,
            tr("failed to check for ID presence").toStdString());
      }
    }

    if (merge_flash_filesystem_ && images_.contains(spiffsBlockOffset)) {
      emit statusMessage(tr("Reading file system image..."), true);

      auto res = mergeFlashLocked(data_port);
      if (res.ok()) {
        images_[spiffsBlockOffset] = res.ValueOrDie();
        emit statusMessage(tr("Merged flash content"), true);
      } else {
        emit statusMessage(tr("Failed to merge flash content: %1")
                               .arg(res.status().ToString().c_str()),
                           true);
        if (!id_generated) {
          return util::Status(
              util::error::UNKNOWN,
              tr("failed to merge flash filesystem").toStdString());
        }
      }
    }

    written_blocks_ = 0;
    for (ulong image_addr : images_.keys()) {
      bool success = false;
      const int written_blocks_before = written_blocks_;
      int offset = 0;
      emit statusMessage(tr("Writing image 0x%1...").arg(image_addr, 0, 16),
                         true);

      for (int attempts = 2; attempts >= 0; attempts--) {
        ulong addr = image_addr + offset;
        QByteArray data = images_[image_addr].mid(offset);
        written_blocks_ = written_blocks_before + (offset / writeBlockSize);
        emit statusMessage(
            tr("Writing %1 bytes @ 0x%2").arg(data.size()).arg(addr, 0, 16),
            true);
        int bytes_written = 0;
        util::Status s =
            writeFlashLocked(data_port, addr, data, &bytes_written);
        success = s.ok();
        if (success) break;
        // Must resume at the nearest 4K boundary, otherwise erasing will fail.
        const int progress = bytes_written - bytes_written % flashBlockSize;
        // Only fail if we made no progress at all after 3 attempts.
        if (progress > 0) attempts = 2;
        qCritical() << s.ToString().c_str();
        qCritical() << "Failed to write image at" << hex << showbase << addr
                    << "," << dec << attempts << "attempts left";
        offset += progress;
        if (!rebootIntoBootloader(port_, invert_dtr_rts_, data_port)) {
          break;
        }
      }
      if (!success) {
        return util::Status(util::error::UNKNOWN,
                            tr("failed to flash image at 0x%1")
                                .arg(image_addr, 0, 16)
                                .toStdString());
      }
    }

    switch ((flashParams >> 8) & 0xff) {
      case 2:  // DIO
        // This is a workaround for ROM switching flash in DIO mode to
        // read-only. See https://github.com/nodemcu/nodemcu-firmware/pull/523
        rebootIntoFirmware(port_, invert_dtr_rts_);
        break;
      default:
        util::Status s = leaveFlashingModeLocked(data_port);
        if (!s.ok()) {
          qCritical() << s.ToString().c_str();
          return util::Status(
              util::error::UNKNOWN,
              tr("Failed to leave flashing mode. Most likely flashing was "
                 "successful, but you need to reboot your device manually.")
                  .toStdString());
        }
        break;
    }

    return util::Status::OK;
  }

  static ulong fixupEraseLength(ulong start, ulong len) {
    // This function allows to offset for SPIEraseArea bug in ESP8266 ROM,
    // making it erase at most one extra 4KB sector.
    //
    // Flash chips used with ESP8266 have 4KB sectors grouped into 64KB blocks.
    // SPI commands allow to erase each sector separately and the whole block at
    // once, so SPIEraseArea attempts to be smart and first erase sectors up to
    // the end of the block, then continue with erasing in blocks and again
    // erase a few sectors in the beginning of the last block.
    // But it does not subtract the number of sectors erased in the first block
    // from the total number of sectors to erase, so that number gets erased
    // twice. Also, due to the way it is written, even if you tell it to erase
    // range starting and ending at the block boundary it will erase first and
    // last block sector by sector.
    // The number of sectors erased is a function of 2 arguments:
    // f(x,t) = 2*x, if x <= t
    //          x+t, if x > t
    // Where `x` - number of sectors to erase, `t` - number of sectors to erase
    // in the first block (that is, 16 if we start at the block boundary).
    // To offset that we don't pass `x` directly, but some function of `x` and
    // `t`:
    // g(x,t) = x/2 + x%1, if x <= 2*t
    //          x-t      , if x > 2*t
    // Results of pieces of `g` fall in the same ranges as inputs of
    // corresponding pieces of `f`, so it's a bit easier to express their
    // composition:
    // f(g(x,t),t) = 2*(x/2 + x%1) = x + x%1, if g(x,t) <= t
    //               (x-t) + t = x          , if g(x,t) > t
    // So, the worst case is when you need to erase odd number of sectors less
    // than `2*t`, then one extra sector will be erased, in all other cases no
    // extra sectors are erased.
    const ulong sectorSize = 4096;
    const ulong sectorsPerBlock = 16;
    start /= sectorSize;
    ulong tail = sectorsPerBlock - start % sectorsPerBlock;
    ulong sectors = len / sectorSize;
    if (len % sectorSize != 0) {
      sectors++;
    }
    if (sectors <= 2 * tail) {
      return (sectors / 2 + (sectors % 2)) * sectorSize;
    } else {
      return len - tail * sectorSize;
    }
  }

  util::Status writeFlashLocked(QSerialPort* port, ulong addr,
                                const QByteArray& bytes, int* bytes_written) {
    const ulong blocks = bytes.length() / writeBlockSize +
                         (bytes.length() % writeBlockSize == 0 ? 0 : 1);
    *bytes_written = 0;
    qDebug() << "Writing" << blocks << "blocks at" << hex << showbase << addr;
    emit statusMessage(tr("Erasing flash at 0x%1...").arg(addr, 0, 16));
    auto s = writeFlashStartLocked(port, addr, blocks);
    if (!s.ok()) {
      return s;
    }
    for (ulong start = 0; start < ulong(bytes.length());
         start += writeBlockSize) {
      QByteArray data = bytes.mid(start, writeBlockSize);
      if (ulong(data.length()) < writeBlockSize) {
        data.append(
            QByteArray("\xff").repeated(writeBlockSize - data.length()));
      }
      qDebug() << "Writing block" << start / writeBlockSize;
      const int block_num = start / writeBlockSize;
      emit statusMessage(tr("Writing block %1 @ 0x%2...")
                             .arg(block_num)
                             .arg(addr + start, 0, 16));
      s = writeFlashBlockLocked(port, block_num, data);
      if (!s.ok()) {
        return util::Status(s.error_code(),
                            QString("Failed to write block %1: %2")
                                .arg(start / writeBlockSize)
                                .arg(s.ToString().c_str())
                                .toStdString());
      }
      written_blocks_++;
      emit progress(written_blocks_);
      *bytes_written += data.length();
    }
    return util::Status::OK;
  }

  util::Status writeFlashStartLocked(QSerialPort* port, ulong addr,
                                     ulong blocks) {
    QByteArray payload;
    QDataStream s(&payload, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    if (erase_bug_workaround_) {
      s << quint32(fixupEraseLength(addr, blocks * writeBlockSize));
    } else {
      s << quint32(blocks * writeBlockSize);
    }
    s << quint32(blocks) << quint32(writeBlockSize) << quint32(addr);
    qDebug() << "Attempting to start flashing...";
    writeCommand(port, 0x02, payload);
    const auto resp = readResponse(port, 30000);
    if (!resp.ok()) {
      return util::Status(
          util::error::ABORTED,
          ("Failed to enter flashing mode, " + resp.error()).toStdString());
    }
    return util::Status::OK;
  }

  util::Status writeFlashBlockLocked(QSerialPort* port, int seq,
                                     const QByteArray& bytes) {
    QByteArray payload;
    QDataStream s(&payload, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << quint32(bytes.length()) << quint32(seq) << quint32(0) << quint32(0);
    payload.append(bytes);
    writeCommand(port, 0x03, payload, checksum(bytes));
    const auto resp = readResponse(port, 10000);
    if (!resp.ok()) {
      return util::Status(util::error::ABORTED, resp.error().toStdString());
    }
    return util::Status::OK;
  }

  util::Status leaveFlashingModeLocked(QSerialPort* port) {
    QByteArray payload("\x01\x00\x00\x00", 4);
    writeCommand(port, 0x04, payload);
    const auto resp = readResponse(port, 10000);
    if (!resp.ok()) {
      qDebug() << "Failed to leave flashing mode." << resp.error();
      if (erase_bug_workaround_) {
        // Error here is expected, Espressif's esptool.py ignores it as well.
        return util::Status::OK;
      }
      return util::Status(util::error::ABORTED, resp.error().toStdString());
    }
    return util::Status::OK;
  }

  util::StatusOr<QByteArray> readFlashLocked(QSerialPort* port, ulong offset,
                                             ulong len) {
    // Init flash.
    util::Status status = writeFlashStartLocked(port, 0, 0);
    if (!status.ok()) {
      return util::Status(util::error::ABORTED,
                          QString("Failed to initialize flash: %1")
                              .arg(status.ToString().c_str())
                              .toStdString());
    }

    QByteArray stub;
    QDataStream s(&stub, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << quint32(offset) << quint32(len) << quint32(1);
    stub.append(ESPReadFlashStub, sizeof(ESPReadFlashStub) - 1);

    QByteArray payload;
    QDataStream s1(&payload, QIODevice::WriteOnly);
    s1.setByteOrder(QDataStream::LittleEndian);
    s1 << quint32(stub.length()) << quint32(1) << quint32(stub.length())
       << quint32(0x40100000);

    writeCommand(port, 0x05, payload);
    auto resp = readResponse(port);
    if (!resp.ok()) {
      qDebug() << "Failed to start writing to RAM." << resp.error();
      return util::Status(util::error::ABORTED,
                          "failed to start writing to RAM");
    }

    payload.clear();
    QDataStream s2(&payload, QIODevice::WriteOnly);
    s2.setByteOrder(QDataStream::LittleEndian);
    s2 << quint32(stub.length()) << quint32(0) << quint32(0) << quint32(0);
    payload.append(stub);
    qDebug() << "Stub length:" << showbase << hex << stub.length();
    writeCommand(port, 0x07, payload, checksum(stub));
    resp = readResponse(port);
    if (!resp.ok()) {
      qDebug() << "Failed to write to RAM." << resp.error();
      return util::Status(util::error::ABORTED, "failed to write to RAM");
    }

    payload.clear();
    QDataStream s3(&payload, QIODevice::WriteOnly);
    s3.setByteOrder(QDataStream::LittleEndian);
    s3 << quint32(0) << quint32(0x4010001c);
    writeCommand(port, 0x06, payload);
    resp = readResponse(port);
    if (!resp.ok()) {
      qDebug() << "Failed to complete writing to RAM." << resp.error();
      return util::Status(util::error::ABORTED, "failed to initialize flash");
    }

    QByteArray r = SLIP_read(port);
    if (r.length() < int(len)) {
      qDebug() << "Failed to read flash.";
      return util::Status(util::error::ABORTED, "failed to read flash");
    }

    if (!trySync(port, 5)) {
      qCritical() << "Device did not reboot after reading flash";
      return util::Status(util::error::ABORTED,
                          "failed to jump to bootloader after reading flash");
    }

    return r;
  }

  // readFlashParamsLocked puts a snippet of code in the RAM and executes it.
  // You need to reboot the device again after that to talk to the bootloader
  // again.
  util::StatusOr<QByteArray> readFlashParamsLocked(QSerialPort* port) {
    auto res = readFlashLocked(port, 0, 4);
    if (!res.ok()) {
      return res.status();
    }

    QByteArray r = res.ValueOrDie();
    if (r[0] != (char) 0xE9) {
      return util::Status(util::error::UNKNOWN,
                          "Read image doesn't seem to have the proper header");
    }
    return r.mid(2, 2);
  }

  // mergeFlashLocked reads the spiffs filesystem from the device
  // and mounts it in memory. Then it overwrites the files that are
  // present in the software update but it leaves the existing ones.
  // The idea is that the filesystem is mostly managed by the user
  // or by the software update utility, while the core system uploaded by
  // the flasher should only upload a few core files.
  util::StatusOr<QByteArray> mergeFlashLocked(QSerialPort* port) {
    auto res = readFlashLocked(port, spiffsBlockOffset, spiffsBlockSize);
    if (!res.ok()) {
      return res.status();
    }

    SPIFFS bundled(images_[spiffsBlockOffset]);
    SPIFFS dev(res.ValueOrDie());

    auto err = dev.merge(bundled);
    if (!err.ok()) {
      return err;
    }
    if (!files_.empty()) {
      err = dev.mergeFiles(files_);
      if (!err.ok()) {
        return err;
      }
    }
    return dev.data();
  }

  util::StatusOr<bool> findIdLocked(QSerialPort* port) {
    // Block with ID has the following structure:
    // 1) 20-byte SHA-1 hash of the payload
    // 2) payload (JSON object)
    // 3) 1-byte terminator ('\0')
    // 4) padding with 0xFF bytes to the block size
    auto res = readFlashLocked(port, idBlockOffset, idBlockSize);
    if (!res.ok()) {
      return res.status();
    }

    QByteArray r = res.ValueOrDie();
    const int SHA1Length = 20;
    QByteArray hash = r.left(SHA1Length);
    int terminator = r.indexOf('\0', SHA1Length);
    if (terminator < 0) {
      return false;
    }
    return hash ==
           QCryptographicHash::hash(r.mid(SHA1Length, terminator - SHA1Length),
                                    QCryptographicHash::Sha1);
  }

  mutable QMutex lock_;
  QMap<ulong, QByteArray> images_;
  QMap<QString, QByteArray> files_;
  QSerialPort* port_;
  int written_blocks_ = 0;
  bool preserve_flash_params_ = true;
  bool erase_bug_workaround_ = true;
  qint32 override_flash_params_ = -1;
  bool merge_flash_filesystem_ = true;
  bool generate_id_if_none_found_ = true;
  QString id_hostname_;
  QString flashing_port_name_;
  int flashing_speed_ = 230400;
  bool invert_dtr_rts_ = false;
};

class ESP8266HAL : public HAL {
  util::Status probe(const QSerialPortInfo& port) const override {
    auto r = connectSerial(port, 9600);
    if (!r.ok()) {
      return r.status();
    }
    std::unique_ptr<QSerialPort> s(r.ValueOrDie());

    // TODO(imax): find a way to pass value of `--esp8266-invert-dtr-rts` here.
    if (!rebootIntoBootloader(s.get(), false)) {
      return util::Status(util::error::ABORTED,
                          "Failed to reboot into bootloader");
    }

    auto mac = read_MAC(s.get()).toHex();
    if (mac.length() < 6) {
      return util::Status(util::error::ABORTED, "Failed to read MAC address");
    }
    qInfo() << "MAC address: " << mac;

    return util::Status::OK;
  }
  std::unique_ptr<Flasher> flasher() const override {
    return std::move(std::unique_ptr<Flasher>(new FlasherImpl));
  }

  std::string name() const override {
    return "ESP8266";
  }

  util::Status reboot(QSerialPort* port) const override {
    // TODO(imax): find a way to pass value of `--esp8266-invert-dtr-rts` here.
    rebootIntoFirmware(port, false);
    return util::Status::OK;
  }
};

}  // namespace

std::unique_ptr<::HAL> HAL() {
  return std::move(std::unique_ptr<::HAL>(new ESP8266HAL));
}

namespace {

using std::map;
using std::string;

const map<string, int> flashMode = {
    {"qio", 0}, {"qout", 1}, {"dio", 2}, {"dout", 3},
};

const map<string, int> flashSize = {
    {"4m", 0},
    {"2m", 1},
    {"8m", 2},
    {"16m", 3},
    {"32m", 4},
    {"16m-c1", 5},
    {"32m-c1", 6},
    {"32m-c2", 7},
};

const map<string, int> flashFreq = {
    {"40m", 0}, {"26m", 1}, {"20m", 2}, {"80m", 0xf},
};
}

util::StatusOr<int> flashParamsFromString(const QString& s) {
  QStringList parts = s.split(',');
  switch (parts.size()) {
    case 1: {  // number
      bool ok = false;
      int r = s.toInt(&ok, 0);
      if (!ok) {
        return util::Status(util::error::INVALID_ARGUMENT, "invalid number");
      }
      return r & 0xffff;
    }
    case 3:  // string
      if (flashMode.find(parts[0].toStdString()) == flashMode.end()) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "invalid flash mode");
      }
      if (flashSize.find(parts[1].toStdString()) == flashSize.end()) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "invalid flash size");
      }
      if (flashFreq.find(parts[2].toStdString()) == flashFreq.end()) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "invalid flash frequency");
      }
      return (flashMode.find(parts[0].toStdString())->second << 8) |
             (flashSize.find(parts[1].toStdString())->second << 4) |
             (flashFreq.find(parts[2].toStdString())->second);
    default:
      return util::Status(
          util::error::INVALID_ARGUMENT,
          "must be either a number or a comma-separated list of three items");
  }
}

void addOptions(QCommandLineParser* parser) {
  parser->addOptions(
      {{kFlashParamsOption,
        "Override params bytes read from existing firmware. Either a "
        "comma-separated string or a number. First component of the string is "
        "the flash mode, must be one of: qio (default), qout, dio, dout. "
        "Second component is flash size, value values: 2m, 4m (default), 8m, "
        "16m, 32m, 16m-c1, 32m-c1, 32m-c2. Third one is flash frequency, valid "
        "values: 40m (default), 26m, 20m, 80m. If it's a number, only 2 lowest "
        "bytes from it will be written in the header of section 0x0000 in "
        "big-endian byte order (i.e. high byte is put at offset 2, low byte at "
        "offset 3).",
        "params"},
       {kSkipReadingFlashParamsOption,
        "If set and --esp8266-flash-params is not used, reading flash params "
        "from the device will not be attempted and image at 0x0000 will be "
        "written as is."},
       {kDisableEraseWorkaroundOption,
        "ROM code can erase up to 16 extra 4KB sectors when flashing firmware. "
        "This flag disables the workaround that makes it erase at most 1 extra "
        "sector."},
       {kFlashingDataPort,
        "If set, communication with ROM will be performed using another serial "
        "port. DTR/RTS signals for rebooting and console will still use the "
        "main port.",
        "port"},
       {kInvertDTRRTS,
        "If set, code that reboots the board will assume that your "
        "USB-to-serial adapter has logical levels for DTR and RTS lines "
        "inverted."}});
}

QByteArray makeIDBlock(const QString& domain) {
  QByteArray data = randomDeviceID(domain);
  QByteArray r = QCryptographicHash::hash(data, QCryptographicHash::Sha1)
                     .append(data)
                     .append("\0", 1);
  r.append(QByteArray("\xFF").repeated(idBlockSize - r.length()));
  return r;
}

}  // namespace ESP8266

#include "esp8266.moc"
