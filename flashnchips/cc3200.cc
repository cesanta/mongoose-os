#include "cc3200.h"

#include <memory>
#include <vector>
#ifdef Q_OS_OSX
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#endif

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#ifndef NO_LIBFTDI
#include <ftdi.h>
#endif

#include <common/util/status.h>
#include <common/util/statusor.h>

#include "fs.h"
#include "serial.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qInfo qWarning
#endif

namespace CC3200 {

namespace {

const int kSerialSpeed = 921600;
const int kVendorID = 0x0451;
const int kProductID = 0xC32A;
const int kDefaultTimeoutMs = 1000;

const int kStorageID = 0;
const char kFWFilename[] = "/sys/mcuimg.bin";
const char kDevConfFilename[] = "/conf/dev.json";
const char kFS0Filename[] = "0.fs";
const char kFS1Filename[] = "1.fs";
const int kBlockSizes[] = {0x100, 0x400, 0x1000, 0x4000, 0x10000};
const int kFileUploadBlockSize = 4096;
const int kSPIFFSMetadataSize = 64;

const char kOpcodeStartUpload = 0x21;
const char kOpcodeFinishUpload = 0x22;

const char kOpcodeFileChunk = 0x24;

const char kOpcodeGetFileInfo = 0x2A;
const char kOpcodeReadFileChunk = 0x2B;

const char kOpcodeStorageWrite = 0x2D;
const char kOpcodeFileErase = 0x2E;
const char kOpcodeGetVersionInfo = 0x2F;

const char kOpcodeEraseBlocks = 0x30;
const char kOpcodeGetStorageInfo = 0x31;
const char kOpcodeExecFromRAM = 0x32;
const char kOpcodeSwitchUART2Apps = 0x33;

struct VersionInfo {
  quint8 byte1;
  quint8 byte16;
};

struct StorageInfo {
  quint16 blockSize;
  quint16 blockCount;
};

struct FileInfo {
  bool exists;
  quint32 size;
};

// Functions for handling the framing protocol. Each frame must be ACKed
// (2 bytes, 00 CC). Frame contains 3 fields:
//  - 2 bytes: big-endian number, length of payload + 2
//  - 1 byte: payload checksum (a sum of all bytes modulo 256)
//  - N bytes: payload
// First byte of each frame sent to the device is an opcode, rest are the
// arguments.

quint8 checksum(const QByteArray& bytes) {
  quint8 r = 0;
  for (const unsigned char c : bytes) {
    r = (r + c) % 0x100;
  }
  return r;
}

util::StatusOr<QByteArray> readBytes(QSerialPort* s, int n,
                                     int timeout = kDefaultTimeoutMs) {
  QByteArray r;
  int i = 0;
  char c = 0;
  while (i < n) {
    if (s->bytesAvailable() == 0 && !s->waitForReadyRead(timeout)) {
      qDebug() << "Read bytes:" << r.toHex();
      return util::Status(
          util::error::DEADLINE_EXCEEDED,
          QString("Timeout on reading byte %1").arg(i).toStdString());
    }
    if (!s->getChar(&c)) {
      qDebug() << "Read bytes:" << r.toHex();
      return util::Status(util::error::UNKNOWN,
                          QString("Error reading byte %1: %2")
                              .arg(i)
                              .arg(s->errorString())
                              .toStdString());
    }
    r.append(c);
    i++;
  }
  qDebug() << "Read bytes:" << r.toHex();
  return r;
}

util::Status writeBytes(QSerialPort* s, const QByteArray& bytes,
                        int timeout = kDefaultTimeoutMs) {
  qDebug() << "Writing bytes:" << bytes.toHex();
  if (!s->write(bytes)) {
    return util::Status(
        util::error::UNKNOWN,
        QString("Write failed: %1").arg(s->errorString()).toStdString());
  }
  if (!s->waitForBytesWritten(timeout)) {
    return util::Status(
        util::error::DEADLINE_EXCEEDED,
        QString("Write timed out: %1").arg(s->errorString()).toStdString());
  }
  return util::Status::OK;
}

util::Status recvAck(QSerialPort* s, int timeout = kDefaultTimeoutMs) {
  auto r = readBytes(s, 2, timeout);
  if (!r.ok()) {
    return r.status();
  }
  if (r.ValueOrDie()[0] != 0 || (unsigned char) (r.ValueOrDie()[1]) != 0xCC) {
    return util::Status(util::error::UNKNOWN,
                        QString("Expected ACK(\\x00\\xCC), got %1")
                            .arg(QString::fromUtf8(r.ValueOrDie().toHex()))
                            .toStdString());
  }
  return util::Status::OK;
}

util::Status sendAck(QSerialPort* s, int timeout = kDefaultTimeoutMs) {
  return writeBytes(s, QByteArray("\x00\xCC", 2), timeout);
}

util::Status doBreak(QSerialPort* s, int timeout = kDefaultTimeoutMs) {
  qInfo() << "Sending break...";
  s->clear();
  if (!s->setBreakEnabled(true)) {
    return util::Status(util::error::UNKNOWN,
                        QString("setBreakEnabled(true) failed: %1")
                            .arg(s->errorString())
                            .toStdString());
  }
  QThread::msleep(500);
  if (!s->setBreakEnabled(false)) {
    return util::Status(util::error::UNKNOWN,
                        QString("setBreakEnabled(false) failed: %1")
                            .arg(s->errorString())
                            .toStdString());
  }
  return recvAck(s, timeout);
}

util::StatusOr<QByteArray> recvPacket(QSerialPort* s,
                                      int timeout = kDefaultTimeoutMs) {
  auto r = readBytes(s, 3, timeout);
  if (!r.ok()) {
    return r.status();
  }
  QDataStream hs(r.ValueOrDie());
  hs.setByteOrder(QDataStream::BigEndian);
  quint16 len;
  quint8 csum;
  hs >> len >> csum;
  auto payload = readBytes(s, len - 2, timeout);
  if (!payload.ok()) {
    return payload.status();
  }
  quint8 pcsum = checksum(payload.ValueOrDie());
  if (csum != pcsum) {
    return util::Status(util::error::UNKNOWN,
                        QString("Invalid checksum: %1, expected %2")
                            .arg(pcsum)
                            .arg(csum)
                            .toStdString());
  }
  sendAck(s, timeout);  // return value ignored
  return payload.ValueOrDie();
}

util::Status sendPacket(QSerialPort* s, const QByteArray& bytes,
                        int timeout = kDefaultTimeoutMs) {
  QByteArray header;
  QDataStream hs(&header, QIODevice::WriteOnly);
  hs.setByteOrder(QDataStream::BigEndian);
  hs << quint16(bytes.length() + 2) << checksum(bytes);
  util::Status st = writeBytes(s, header, timeout);
  if (!st.ok()) {
    return st;
  }
  st = writeBytes(s, bytes, timeout);
  if (!st.ok()) {
    return st;
  }
  return recvAck(s, timeout);
}

#ifndef NO_LIBFTDI
util::StatusOr<ftdi_context*> openFTDI() {
  std::unique_ptr<ftdi_context, void (*) (ftdi_context*) > ctx(ftdi_new(),
                                                               ftdi_free);
  if (ftdi_set_interface(ctx.get(), INTERFACE_A) != 0) {
    return util::Status(util::error::UNKNOWN, "ftdi_set_interface failed");
  }
  if (ftdi_usb_open(ctx.get(), kVendorID, kProductID) != 0) {
    return util::Status(util::error::UNKNOWN, "ftdi_usb_open failed");
  }
  if (ftdi_write_data_set_chunksize(ctx.get(), 1) != 0) {
    return util::Status(util::error::UNKNOWN,
                        "ftdi_write_data_set_chunksize failed");
  }
  if (ftdi_set_bitmode(ctx.get(), 0x61, BITMODE_BITBANG) != 0) {
    return util::Status(util::error::UNKNOWN, "ftdi_set_bitmode failed");
  }
  return ctx.release();
}

util::Status resetSomething(ftdi_context* ctx) {
  unsigned char c = 1;
  if (ftdi_write_data(ctx, &c, 1) < 0) {
    return util::Status(util::error::UNKNOWN, "ftdi_write_data failed");
  }
  QThread::msleep(5);
  c |= 0x20;
  if (ftdi_write_data(ctx, &c, 1) < 0) {
    return util::Status(util::error::UNKNOWN, "ftdi_write_data failed");
  }
  QThread::msleep(1000);
  return util::Status::OK;
}

util::Status boot(ftdi_context* ctx) {
  util::Status st;
  const std::vector<unsigned char> seq = {0, 0x20};
  for (unsigned char b : seq) {
    if (ftdi_write_data(ctx, &b, 1) < 0) {
      return util::Status(util::error::UNKNOWN, "ftdi_write_data failed");
    }
    QThread::msleep(100);
  }
  return util::Status::OK;
}
#endif

class FlasherImpl : public Flasher {
  Q_OBJECT
 public:
  FlasherImpl(){};
  util::Status load(const QString& path) override {
    QMutexLocker lock(&lock_);
    QDir dir(path, "*.bin", QDir::Name,
             QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    if (!dir.exists()) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          tr("Directory does not exist").toStdString());
    }
    const auto files = dir.entryInfoList();
    if (files.length() == 0) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          tr("No files to flash").toStdString());
    }
    if (files.length() > 1) {
      return util::Status(
          util::error::FAILED_PRECONDITION,
          tr("There must be exactly 1 *.bin file").toStdString());
    }
    QFile f(files[0].absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
      return util::Status(util::error::ABORTED,
                          tr("Failed to open %1")
                              .arg(files[0].absoluteFilePath())
                              .toStdString());
    }
    image_ = f.readAll();
    if (image_.length() != files[0].size()) {
      const int len = image_.length();
      image_.clear();
      return util::Status(util::error::UNAVAILABLE,
                          tr("%1 has size %2, but readAll returned %3 bytes")
                              .arg(files[0].fileName())
                              .arg(files[0].size())
                              .arg(len)
                              .toStdString());
    }
    const int kMaxSize =
        kBlockSizes[sizeof(kBlockSizes) / sizeof(kBlockSizes[0]) - 1] * 255;
    if (image_.length() > kMaxSize) {
      image_.clear();
      return util::Status(util::error::INVALID_ARGUMENT,
                          tr("%1 is too big. Maximum allowed size is %2")
                              .arg(files[0].fileName())
                              .arg(kMaxSize)
                              .toStdString());
    }
    return loadSPIFFS(path);
  }

  util::Status setPort(QSerialPort* port) override {
    QMutexLocker lock(&lock_);
    port_ = port;
    int speed = kSerialSpeed;
    if (!port_->setBaudRate(speed)) {
      return util::Status(
          util::error::INTERNAL,
          QCoreApplication::translate("connectSerial",
                                      "Failed to set baud rate").toStdString());
    }
#ifdef Q_OS_OSX
    if (ioctl(port_->handle(), IOSSIOSPEED, &speed) < 0) {
      return util::Status(
          util::error::INTERNAL,
          QCoreApplication::translate("connectSerial",
                                      "Failed to set baud rate with ioctl")
              .toStdString());
    }
#endif
    return util::Status::OK;
  }

  int totalBlocks() const override {
    QMutexLocker lock(&lock_);
    int r = image_.length() / kFileUploadBlockSize +
            (image_.length() % kFileUploadBlockSize > 0 ? 1 : 0);
    if (spiffs_image_.length() > 0) {
      const int bytes = spiffs_image_.length() + kSPIFFSMetadataSize;
      r += bytes / kFileUploadBlockSize;
      if (bytes % kFileUploadBlockSize > 0) {
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

  util::Status setOption(const QString& name, const QVariant& value) override {
    if (name == kIdDomainOption) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      id_hostname_ = value.toString();
      return util::Status::OK;
    } else if (name == kSkipIdGenerationOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      skip_id_generation_ = value.toBool();
      return util::Status::OK;
    } else if (name == kOverwriteFSOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      overwrite_spiffs_ = value.toBool();
      return util::Status::OK;
    }
    return util::Status(util::error::INVALID_ARGUMENT, "Unknown option");
  }

  util::Status setOptionsFromCommandLine(
      const QCommandLineParser& parser) override {
    util::Status r;

    QStringList boolOpts({kSkipIdGenerationOption, kOverwriteFSOption});
    QStringList stringOpts({kIdDomainOption});

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

 private:
  util::Status loadSPIFFS(const QString& path) {
    spiffs_image_.clear();
    QDir dir(path, "fs.img", QDir::Name,
             QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    const auto files = dir.entryInfoList();
    if (files.length() == 0) {
      // No FS image, nothing to do.
      return util::Status::OK;
    }
    QFile f(files[0].absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
      return util::Status(util::error::ABORTED,
                          tr("Failed to open %1")
                              .arg(files[0].absoluteFilePath())
                              .toStdString());
    }
    spiffs_image_ = f.readAll();
    if (spiffs_image_.length() != files[0].size()) {
      const int len = spiffs_image_.length();
      spiffs_image_.clear();
      return util::Status(util::error::UNAVAILABLE,
                          tr("%1 has size %2, but readAll returned %3 bytes")
                              .arg(files[0].fileName())
                              .arg(files[0].size())
                              .arg(len)
                              .toStdString());
    }
    return util::Status::OK;
  }

  util::Status runLocked() {
    util::Status st;
    progress_ = 0;
    emit progress(progress_);
#ifndef NO_LIBFTDI
    emit statusMessage(tr("Opening FTDI context..."), true);
    auto r = openFTDI();
    if (!r.ok()) {
      return r.status();
    }
    std::unique_ptr<ftdi_context, void (*) (ftdi_context*) > ctx(r.ValueOrDie(),
                                                                 ftdi_free);
    emit statusMessage(tr("Resetting the device..."), true);
    st = resetSomething(ctx.get());
    if (!st.ok()) {
      return st;
    }
#endif
    st = doBreak(port_);
    if (!st.ok()) {
      return st;
    }
    emit statusMessage(tr("Updating bootloader..."), true);
    st = switchToNWPBootloader();
    if (!st.ok()) {
      return st;
    }
    emit statusMessage(tr("Uploading the firmware image..."), true);
    st = uploadFW(image_, kFWFilename);
    if (!st.ok()) {
      return st;
    }
    if (!skip_id_generation_) {
      emit statusMessage(tr("Checking if the device has an ID assigned..."),
                         true);
      auto info = getFileInfo(kDevConfFilename);
      if (!info.ok()) {
        return info.status();
      }
      if (!info.ValueOrDie().exists) {
        emit statusMessage(tr("Generating a new ID..."), true);
        QByteArray id = randomDeviceID(id_hostname_);
        st = uploadFW(id, kDevConfFilename);
        if (!st.ok()) {
          return st;
        }
      }
    }
    if (spiffs_image_.length() > 0) {
      emit statusMessage(tr("Updating file system image..."), true);
      st = updateSPIFFS();
      if (!st.ok()) {
        return st;
      }
    }
#ifndef NO_LIBFTDI
    emit statusMessage(tr("Rebooting into firmware..."), true);
    st = boot(ctx.get());
    if (!st.ok()) {
      return st;
    }
#endif
    return util::Status::OK;
  }

  static int getBlockSize(int len) {
    for (unsigned int i = 0; i < sizeof(kBlockSizes) / sizeof(kBlockSizes[0]);
         i++) {
      if (len <= kBlockSizes[i] * 255) {
        return kBlockSizes[i];
      }
    }
    return -1;
  }

  util::StatusOr<VersionInfo> getVersion() {
    emit statusMessage(tr("Getting device version info..."));
    util::Status st = sendPacket(port_, QByteArray(&kOpcodeGetVersionInfo, 1));
    if (!st.ok()) {
      return st;
    }
    auto data = recvPacket(port_);
    if (!data.ok()) {
      return data.status();
    }
    if (data.ValueOrDie().length() != 28) {
      return util::Status(util::error::UNKNOWN,
                          QString("Expected 28 bytes, got %1")
                              .arg(data.ValueOrDie().length())
                              .toStdString());
    }
    return VersionInfo(
        {quint8(data.ValueOrDie()[1]), quint8(data.ValueOrDie()[16])});
  }

  util::StatusOr<StorageInfo> getStorageInfo() {
    emit statusMessage(tr("Getting storage info..."));
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeGetStorageInfo) << quint32(kStorageID);
    auto st = sendPacket(port_, payload);
    if (!st.ok()) {
      return st;
    }
    auto resp = recvPacket(port_);
    if (!resp.ok()) {
      return resp.status();
    }
    if (resp.ValueOrDie().length() < 4) {
      return util::Status(util::error::UNKNOWN,
                          QString("Expected at least 4 bytes, got %1")
                              .arg(resp.ValueOrDie().length())
                              .toStdString());
    }
    StorageInfo r;
    QDataStream rs(resp.ValueOrDie());
    rs.setByteOrder(QDataStream::BigEndian);
    rs >> r.blockSize >> r.blockCount;
    return r;
  }

  util::Status eraseBlocks(int start, int count) {
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeEraseBlocks) << quint32(kStorageID) << quint32(start)
       << quint32(count);
    return sendPacket(port_, payload);
  }

  util::Status sendChunk(int offset, const QByteArray& bytes) {
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeStorageWrite) << quint32(kStorageID) << quint32(offset)
       << quint32(bytes.length());
    payload.append(bytes);
    return sendPacket(port_, payload);
  }

  util::Status rawWrite(quint32 offset, const QByteArray& bytes) {
    auto si = getStorageInfo();
    if (!si.ok()) {
      return si.status();
    }
    if (si.ValueOrDie().blockSize > 0) {
      quint16 bs = si.ValueOrDie().blockSize;
      int start = offset / bs;
      int count = bytes.length() / bs;
      if ((bytes.length() % bs) > 0) {
        count++;
      }
      util::Status st = eraseBlocks(start, count);
      if (!st.ok()) {
        return st;
      }
    }
    const int kChunkSize = 4080;
    int sent = 0;
    while (sent < bytes.length()) {
      util::Status st = sendChunk(offset + sent, bytes.mid(sent, kChunkSize));
      if (!st.ok()) {
        return st;
      }
      sent += kChunkSize;
    }
    return util::Status::OK;
  }

  util::Status execFromRAM() {
    return sendPacket(port_, QByteArray(&kOpcodeExecFromRAM, 1));
  }

  util::Status switchUART2Apps() {
    const quint32 magic = 0x0196E6AB;
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeSwitchUART2Apps) << quint32(magic);
    return sendPacket(port_, payload);
  }

  util::Status switchToNWPBootloader() {
    emit statusMessage(tr("Switching to NWP bootloader..."));
    auto ver = getVersion();
    if (!ver.ok()) {
      return ver.status();
    }
    if ((ver.ValueOrDie().byte16 & 0x10) == 0) {
      return util::Status::OK;
    }
    quint8 bl_ver = ver.ValueOrDie().byte1;
    if (bl_ver < 3) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          "Unsupported device");
    } else if (bl_ver == 3) {
      emit statusMessage(tr("Uploading rbtl3101_132.dll..."));
      QFile f(":/cc3200/rbtl3101_132.dll");
      if (!f.open(QIODevice::ReadOnly)) {
        return util::Status(util::error::INTERNAL,
                            "Failed to open embedded file");
      }
      util::Status st = rawWrite(0x4000, f.readAll());
      if (!st.ok()) {
        return st;
      }
      st = execFromRAM();
      if (!st.ok()) {
        return st;
      }
    } else /* if (bl_ver > 3) */ {
      util::Status st = switchUART2Apps();
      if (!st.ok()) {
        return st;
      }
    }
    util::Status st;
    for (int attempt = 0; attempt < 3; attempt++) {
      QThread::sleep(1);
      qInfo() << "Checking if the device is back online...";
      st = doBreak(port_);
      if (st.ok()) {
        break;
      }
    }
    if (!st.ok()) {
      return st;
    }
    QByteArray blob;
    if (bl_ver == 3) {
      emit statusMessage(tr("Uploading rbtl3100.dll..."));
      QFile f(":/cc3200/rbtl3100.dll");
      if (!f.open(QIODevice::ReadOnly)) {
        return util::Status(util::error::INTERNAL,
                            "Failed to open embedded file");
      }
      blob = f.readAll();
    } else /* if (bl_ver > 3) */ {
      emit statusMessage(tr("Uploading rbtl3100s.dll..."));
      QFile f(":/cc3200/rbtl3100s.dll");
      if (!f.open(QIODevice::ReadOnly)) {
        return util::Status(util::error::INTERNAL,
                            "Failed to open embedded file");
      }
      blob = f.readAll();
    }
    st = rawWrite(0, blob);
    if (!st.ok()) {
      return st;
    }
    st = execFromRAM();
    if (!st.ok()) {
      return st;
    }
    return recvAck(port_);
  }

  util::Status eraseFile(const QString& name) {
    emit statusMessage(tr("Erasing %1...").arg(name));
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeFileErase) << quint32(0);
    payload.append(name.toUtf8());
    payload.append('\0');
    return sendPacket(port_, payload);
  }

  util::Status openFileForWrite(const QString& filename, int len) {
    emit statusMessage(tr("Uploading %1 (%2 bytes)...").arg(filename).arg(len),
                       true);
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    quint32 flags = 0x3000;
    const int num_sizes = sizeof(kBlockSizes) / sizeof(kBlockSizes[0]);
    int block_size_index = 0;
    for (; block_size_index < num_sizes; block_size_index++) {
      if (kBlockSizes[block_size_index] * 255 >= len) {
        break;
      }
    }
    if (block_size_index == num_sizes) {
      return util::Status(util::error::FAILED_PRECONDITION, "File is too big");
    }
    flags |= (block_size_index & 0xf) << 8;
    int blocks = len / kBlockSizes[block_size_index];
    if (len % kBlockSizes[block_size_index] > 0) {
      blocks++;
    }
    flags |= blocks & 0xff;
    ps << quint8(kOpcodeStartUpload) << quint32(flags) << quint32(0);
    payload.append(filename.toUtf8());
    payload.append('\0');
    payload.append('\0');
    util::Status st = sendPacket(port_, payload, 10000);
    if (!st.ok()) {
      return st;
    }
    auto token = readBytes(port_, 4);
    if (!token.ok()) {
      return token.status();
    }
    return util::Status::OK;
  }

  util::Status openFileForRead(const QString& filename) {
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeStartUpload) << quint32(0) << quint32(0);
    payload.append(filename.toUtf8());
    payload.append('\0');
    payload.append('\0');
    util::Status st = sendPacket(port_, payload, 10000);
    if (!st.ok()) {
      return st;
    }
    auto token = readBytes(port_, 4);
    if (!token.ok()) {
      return token.status();
    }
    return util::Status::OK;
  }

  util::Status closeFile() {
    QByteArray payload(&kOpcodeFinishUpload, 1);
    payload.append(QByteArray("\0", 1).repeated(63));
    payload.append(QByteArray("\x46", 1).repeated(256));
    payload.append("\0", 1);
    return sendPacket(port_, payload);
  }

  util::Status uploadFW(const QByteArray& bytes, const QString& filename) {
    auto info = getFileInfo(filename);
    if (!info.ok()) {
      return info.status();
    }
    util::Status st;
    if (info.ValueOrDie().exists) {
      st = eraseFile(filename);
      if (!st.ok()) {
        return st;
      }
    }
    st = openFileForWrite(filename, bytes.length());
    if (!st.ok()) {
      return st;
    }
    int start = 0;
    while (start < bytes.length()) {
      emit statusMessage(tr("Sending chunk 0x%1...").arg(start, 0, 16));
      QByteArray payload;
      QDataStream ps(&payload, QIODevice::WriteOnly);
      ps.setByteOrder(QDataStream::BigEndian);
      ps << quint8(kOpcodeFileChunk) << quint32(start);
      payload.append(bytes.mid(start, kFileUploadBlockSize));

      st = sendPacket(port_, payload);
      if (!st.ok()) {
        return st;
      }
      start += kFileUploadBlockSize;
      progress_++;
      emit progress(progress_);
    }
    return closeFile();
  }

  util::StatusOr<FileInfo> getFileInfo(const QString& filename) {
    QByteArray payload;
    QDataStream ps(&payload, QIODevice::WriteOnly);
    ps.setByteOrder(QDataStream::BigEndian);
    ps << quint8(kOpcodeGetFileInfo) << quint32(filename.length());
    payload.append(filename.toUtf8());
    util::Status st = sendPacket(port_, payload);
    if (!st.ok()) {
      return st;
    }
    auto resp = recvPacket(port_);
    if (!resp.ok()) {
      return resp.status();
    }
    QByteArray b = resp.ValueOrDie();
    FileInfo r;
    r.exists = b[0] == char(1);
    QDataStream ss(b.mid(4, 4));
    ss.setByteOrder(QDataStream::BigEndian);
    ss >> r.size;
    return r;
  }

  util::StatusOr<QByteArray> getFile(const QString& filename) {
    auto info = getFileInfo(filename);
    if (!info.ok()) {
      return info.status();
    }
    if (!info.ValueOrDie().exists) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          "File does not exist");
    }
    util::Status st = openFileForRead(filename);
    if (!st.ok()) {
      return st;
    }
    int size = info.ValueOrDie().size;
    QByteArray r;
    while (r.length() < size) {
      int n = kFileUploadBlockSize;
      if (n > size - r.length()) {
        n = size - r.length();
      }

      QByteArray payload;
      QDataStream ps(&payload, QIODevice::WriteOnly);
      ps << quint8(kOpcodeReadFileChunk) << quint32(r.length()) << quint32(n);
      st = sendPacket(port_, payload);
      if (!st.ok()) {
        qCritical() << "getChunk failed at " << r.length() << ": "
                    << st.ToString().c_str();
        return st;
      }
      auto resp = recvPacket(port_);
      if (!resp.ok()) {
        qCritical() << "Failed to read chunk at " << r.length() << ": "
                    << resp.status().ToString().c_str();
        return resp.status();
      }
      r.append(resp.ValueOrDie());
    }

    st = closeFile();
    if (!st.ok()) {
      return st;
    }
    return r;
  }

  util::StatusOr<QByteArray> readSPIFFS(const QString& filename, quint64* seq,
                                        quint32* block_size,
                                        quint32* page_size) {
    auto info = getFileInfo(filename);
    if (!info.ok()) {
      return info.status();
    }
    if (!info.ValueOrDie().exists) {
      return QByteArray();
    }
    auto data = getFile(filename);
    if (!data.ok()) {
      return data.status();
    }
    QByteArray bytes = data.ValueOrDie();
    if (bytes.length() < kSPIFFSMetadataSize) {
      return util::Status(util::error::FAILED_PRECONDITION,
                          "Image is too short");
    }
    QDataStream meta(bytes.mid(bytes.length() - kSPIFFSMetadataSize));
    meta.setByteOrder(QDataStream::BigEndian);
    quint32 fs_size;
    // See struct fs_info in platforms/cc3200/cc3200_fs_spiffs_container.c
    meta >> *seq >> fs_size >> *block_size >> *page_size;
    return bytes.mid(0, bytes.length() - kSPIFFSMetadataSize);
  }

  util::Status updateSPIFFS() {
    quint64 seq[2] = {~(0ULL), ~(0ULL)};
    quint32 page_size[2] = {0, 0}, block_size[2] = {0, 0};
    auto fs0 = readSPIFFS(kFS0Filename, &seq[0], &block_size[0], &page_size[0]);
    if (!fs0.ok()) {
      return fs0.status();
    }
    auto fs1 = readSPIFFS(kFS1Filename, &seq[1], &block_size[1], &page_size[1]);
    if (!fs1.ok()) {
      return fs1.status();
    }
    qInfo() << "Sequence nubmer of 0.fs:" << seq[0];
    qInfo() << "Sequence nubmer of 1.fs:" << seq[1];
    QByteArray meta;
    int min_seq = 0;
    QDataStream ms(&meta, QIODevice::WriteOnly);
    ms.setByteOrder(QDataStream::BigEndian);
    if (seq[0] < seq[1]) {
      ms << seq[0] - 1;
      min_seq = 0;
    } else {
      ms << seq[1] - 1;
      min_seq = 1;
    }
    ms << quint32(spiffs_image_.length());
    // TODO(imax): make mkspiffs write page size and block size into a separate
    // file and use it here instead of hardcoded values.
    ms << quint32(FLASH_BLOCK_SIZE) << quint32(LOG_PAGE_SIZE);
    meta.append(
        QByteArray("\xFF", 1).repeated(kSPIFFSMetadataSize - meta.length()));
    QByteArray image = spiffs_image_;
    if ((fs0.ValueOrDie().length() > 0 || fs1.ValueOrDie().length() > 0) &&
        !overwrite_spiffs_) {
      SPIFFS bundled(spiffs_image_);
      SPIFFS dev(min_seq == 0 ? fs0.ValueOrDie() : fs1.ValueOrDie());

      util::Status st = dev.merge(bundled);
      if (!st.ok()) {
        return st;
      }

      image = dev.data();
    }
    image.append(meta);
    QString fname = min_seq == 0 ? kFS1Filename : kFS0Filename;
    qInfo() << "Overwriting" << fname;
    return uploadFW(image, fname);
  }

  mutable QMutex lock_;
  QByteArray image_;
  QByteArray spiffs_image_;
  QSerialPort* port_;
  QString id_hostname_;
  bool skip_id_generation_ = false;
  bool overwrite_spiffs_ = false;
  int progress_ = 0;
};

class CC3200HAL : public HAL {
  util::Status probe(const QSerialPortInfo& port) const override {
    auto r = connectSerial(port, kSerialSpeed);
    if (!r.ok()) {
      return r.status();
    }
#ifndef NO_LIBFTDI
    auto ftdi = openFTDI();
    if (!ftdi.ok()) {
      return ftdi.status();
    }
    std::unique_ptr<ftdi_context, void (*) (ftdi_context*) > ctx(
        ftdi.ValueOrDie(), ftdi_free);
    util::Status st = resetSomething(ctx.get());
    if (!st.ok()) {
      return st;
    }
#endif
    std::unique_ptr<QSerialPort> s(r.ValueOrDie());
    return doBreak(s.get());
  }

  std::unique_ptr<Flasher> flasher() const override {
    return std::move(std::unique_ptr<Flasher>(new FlasherImpl));
  }

  std::string name() const override {
    return "CC3200";
  }

  util::Status reboot(QSerialPort*) const override {
#ifdef NO_LIBFTDI
    return util::Status(util::error::UNIMPLEMENTED,
                        "Rebooting CC3200 is not supported");
#else   // NO_LIBFTDI
    auto ftdi = openFTDI();
    if (!ftdi.ok()) {
      return ftdi.status();
    }
    std::unique_ptr<ftdi_context, void (*) (ftdi_context*) > ctx(
        ftdi.ValueOrDie(), ftdi_free);
    return boot(ctx.get());
#endif  // NO_LIBFTDI
  }
};

}  // namespace

std::unique_ptr<::HAL> HAL() {
  return std::move(std::unique_ptr<::HAL>(new CC3200HAL));
}

}  // namespace CC3200

#include "cc3200.moc"
