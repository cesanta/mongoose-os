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

#include "config.h"
#include "esp_flasher_client.h"
#include "esp_rom_client.h"
#include "fs.h"
#include "serial.h"
#include "status_qt.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qInfo qWarning
#endif

namespace ESP8266 {

namespace {

const char kFlashParamsOption[] = "esp8266-flash-params";
const char kSkipReadingFlashParamsOption[] =
    "esp8266-skip-reading-flash-params";
const char kFlashingDataPortOption[] = "esp8266-flashing-data-port";
const char kSPIFFSOffsetOption[] = "esp8266-spiffs-offset";
const char kDefaultSPIFFSOffset[] = "0xec000";
const char kSPIFFSSizeOption[] = "esp8266-spiffs-size";
const char kDefaultSPIFFSSize[] = "65536";
const char kNoMinimizeWritesOption[] = "esp8266-no-minimize-writes";

const int kDefaultFlashBaudRate = 230400;
const ulong flashBlockSize = 4096;
const char fwFileGlob[] = "0x*.bin";
const ulong idBlockOffset = 0x10000;
const ulong idBlockSize = flashBlockSize;

enum ESP8266ROMCommand {
  cmdRAMWriteStart = 0x05,
  cmdRAMWriteFinish = 0x06,
  cmdRAMWriteBlock = 0x07,
  cmdSync = 0x08,
  cmdReadReg = 0x0A,
};

class FlasherImpl : public Flasher {
  Q_OBJECT
 public:
  FlasherImpl(Prompter *prompter)
      : prompter_(prompter), id_hostname_("api.cesanta.com") {
  }

  util::Status setOption(const QString &name, const QVariant &value) override {
    if (name == kIdDomainOption) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      id_hostname_ = value.toString();
      return util::Status::OK;
    } else if (name == kMergeFSOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      merge_flash_filesystem_ = value.toBool();
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
    } else if (name == kSkipReadingFlashParamsOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      preserve_flash_params_ = !value.toBool();
      return util::Status::OK;
    } else if (name == kFlashingDataPortOption) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      flashing_port_name_ = value.toString();
      return util::Status::OK;
    } else if (name == kFlashBaudRateOption) {
      if (value.type() != QVariant::Int) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a positive integer");
      }
      flashing_speed_ = value.toInt();
      if (flashing_speed_ <= 0) {
        flashing_speed_ = kDefaultFlashBaudRate;
      }
      return util::Status::OK;
    } else if (name == kDumpFSOption) {
      if (value.type() != QVariant::String) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a string");
      }
      fs_dump_filename_ = value.toString();
      return util::Status::OK;
    } else if (name == kSPIFFSOffsetOption) {
      if (value.type() != QVariant::Int || value.toInt() <= 0) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a positive integer");
      }
      spiffs_offset_ = value.toInt();
      return util::Status::OK;
    } else if (name == kSPIFFSSizeOption) {
      if (value.type() != QVariant::Int || value.toInt() <= 0) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be a positive integer");
      }
      spiffs_size_ = value.toInt();
      return util::Status::OK;
    } else if (name == kNoMinimizeWritesOption) {
      if (value.type() != QVariant::Bool) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            "value must be boolean");
      }
      minimize_writes_ = !value.toBool();
      return util::Status::OK;
    } else {
      return util::Status(util::error::INVALID_ARGUMENT, "unknown option");
    }
  }

  util::Status setOptionsFromConfig(const Config &config) override {
    util::Status r;

    QStringList boolOpts({kMergeFSOption, kNoMinimizeWritesOption,
                          kSkipIdGenerationOption,
                          kSkipReadingFlashParamsOption});
    for (const auto &opt : boolOpts) {
      auto s = setOption(opt, config.isSet(opt));
      if (!s.ok()) {
        return util::Status(
            s.error_code(),
            (opt + ": " + s.error_message().c_str()).toStdString());
      }
    }

    QStringList stringOpts({kIdDomainOption, kFlashParamsOption,
                            kFlashingDataPortOption, kDumpFSOption});
    for (const auto &opt : stringOpts) {
      // XXX: currently there's no way to "unset" a string option.
      if (config.isSet(opt)) {
        auto s = setOption(opt, config.value(opt));
        if (!s.ok()) {
          return util::Status(
              s.error_code(),
              (opt + ": " + s.error_message().c_str()).toStdString());
        }
      }
    }

    QStringList intOpts(
        {kFlashBaudRateOption, kSPIFFSOffsetOption, kSPIFFSSizeOption});
    for (const auto &opt : intOpts) {
      bool ok;
      int value = config.value(opt).toInt(&ok, 0);
      if (!ok) {
        return util::Status(util::error::INVALID_ARGUMENT,
                            (opt + ": Invalid numeric value.").toStdString());
      }
      auto s = setOption(opt, value);
      if (!s.ok()) {
        return util::Status(
            s.error_code(),
            (opt + ": " + s.error_message().c_str()).toStdString());
      }
    }
    return util::Status::OK;
  }

  util::Status load(const QString &path) override {
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
    for (const auto &file : files) {
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
      for (const auto &file : files_dir.entryInfoList()) {
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

  util::Status setPort(QSerialPort *port) override {
    QMutexLocker lock(&lock_);
    port_ = port;
    return util::Status::OK;
  }

  int totalBytes() const override {
    QMutexLocker lock(&lock_);
    int r = 0;
    for (const auto &bytes : images_.values()) {
      r += bytes.length();
    }
    // Add FS once again for reading.
    if (merge_flash_filesystem_ && images_.contains(spiffs_offset_)) {
      r += images_[spiffs_offset_].length();
    }
    return r;
  }

  util::StatusOr<std::unique_ptr<QSerialPort>> getFlashingDataPort() {
    std::unique_ptr<QSerialPort> second_port;
    if (!flashing_port_name_.isEmpty()) {
      const auto &ports = QSerialPortInfo::availablePorts();
      QSerialPortInfo info;
      bool found = false;
      for (const auto &port : ports) {
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
    }

    return std::move(second_port);
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
    progress_ = 0;
    emit progress(progress_);

    auto fdps = getFlashingDataPort();
    if (!fdps.ok()) {
      return QSP("failed to open flashing data port", fdps.status());
    }

    ESPROMClient rom(port_,
                     fdps.ValueOrDie().get() ? fdps.ValueOrDie().get() : port_);

    emit statusMessage("Connecting to ROM...", true);

    util::Status st;
    while (true) {
      st = rom.connect();
      if (st.ok()) break;
      qCritical() << st;
      QString msg =
          tr("Failed to talk to bootloader. See <a "
             "href=\"https://github.com/cesanta/smart.js/blob/master/"
             "smartjs/platforms/esp8266/flashing.md\">wiring instructions</a>. "
             "Alternatively, put the device into flashing mode manually "
             "and retry now.\n\nError: %1")
              .arg(QString::fromUtf8(st.ToString().c_str()));
      int answer =
          prompter_->Prompt(msg, {{tr("Retry"), QMessageBox::NoRole},
                                  {tr("Cancel"), QMessageBox::YesRole}});
      if (answer == 1) {
        return util::Status(util::error::UNAVAILABLE,
                            "Failed to talk to bootloader.");
      }
    }

    emit statusMessage("Running flasher...", true);

    ESPFlasherClient flasher_client(&rom);

    st = flasher_client.connect(flashing_speed_);
    if (!st.ok()) {
      return QSP("Failed to run and communicate with flasher stub", st);
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
      util::StatusOr<QByteArray> r = readFlashParamsLocked(&flasher_client);
      if (!r.ok()) {
        emit statusMessage(tr("Failed to read flash params: %1")
                               .arg(r.status().error_message().c_str()),
                           true);
        QString msg = tr("Failed to read flash params: %1. Continue anyway?")
                          .arg(r.status().error_message().c_str());
        int answer = prompter_->Prompt(msg, {{tr("Yes"), QMessageBox::YesRole},
                                             {tr("No"), QMessageBox::NoRole}});
        if (answer == 1) {
          return util::Status(
              util::error::UNKNOWN,
              tr("Failed to read flash params from the existing firmware")
                  .toStdString());
        }
        QByteArray safeParams(2, 0);
        safeParams[0] = 0x02;  // DIO;
        safeParams[1] = 0;     // 40 MHz
        r = safeParams;
      }
      const QByteArray params = r.ValueOrDie();
      emit statusMessage(
          tr("Current flash params bytes: %1").arg(QString(params.toHex())),
          true);
      flashParams = params[0] << 8 | params[1];
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
      auto res = findIdLocked(&flasher_client);
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
        return QSP("failed to check for ID presence", st);
      }
    }

    qInfo() << QString("SPIFFS params: %1 @ 0x%2")
                   .arg(spiffs_size_)
                   .arg(spiffs_offset_, 0, 16)
                   .toUtf8();
    if (merge_flash_filesystem_ && images_.contains(spiffs_offset_)) {
      auto res = mergeFlashLocked(&flasher_client);
      if (res.ok()) {
        if (res.ValueOrDie().size() > 0) {
          images_[spiffs_offset_] = res.ValueOrDie();
        } else {
          images_.remove(spiffs_offset_);
        }
        emit statusMessage(tr("Merged flash content"), true);
      } else {
        emit statusMessage(tr("Failed to merge flash content: %1")
                               .arg(res.status().ToString().c_str()),
                           true);
        // Temporary: SPIFFS compatibility was broken between 0.3.3 and 0.3.4,
        // overwrite FS for now.
        if (!id_generated) {
          return util::Status(
              util::error::UNKNOWN,
              tr("failed to merge flash filesystem").toStdString());
        }
      }
    } else if (merge_flash_filesystem_) {
      qInfo() << "No SPIFFS image in new firmware";
    }

    auto flashImages =
        minimize_writes_ ? dedupImages(&flasher_client, images_) : images_;

    for (ulong image_addr : flashImages.keys()) {
      QByteArray data = flashImages[image_addr];
      emit progress(progress_);
      int origLength = data.length();

      if (data.length() % flasher_client.flashSectorSize != 0) {
        quint32 padLen = flasher_client.flashSectorSize -
                         (data.length() % flasher_client.flashSectorSize);
        data.reserve(data.length() + padLen);
        while (padLen-- > 0) data.append('\x00');
      }

      emit statusMessage(
          tr("Erasing %1 @ 0x%2...").arg(data.length()).arg(image_addr, 0, 16),
          true);
      st = flasher_client.erase(image_addr, data.length());

      if (st.ok()) {
        emit statusMessage(tr("Writing %1 @ 0x%2...")
                               .arg(data.length())
                               .arg(image_addr, 0, 16),
                           true);
        connect(&flasher_client, &ESPFlasherClient::progress,
                [this, origLength](int bytesWritten) {
                  emit progress(this->progress_ +
                                std::min(bytesWritten, origLength));
                });
        st = flasher_client.write(image_addr, data, false /* erase */);
        disconnect(&flasher_client, &ESPFlasherClient::progress, 0, 0);
      }
      if (!st.ok()) {
        return QS(util::error::UNAVAILABLE,
                  tr("failed to flash image at 0x%1: %2")
                      .arg(image_addr, 0, 16)
                      .arg(st.ToString().c_str()));
      }
      progress_ += origLength;
    }

    st = verifyImages(&flasher_client, images_);
    if (!st.ok()) return QSP("verification failed", st);

    // Ideally, we'd like to tell flasher stub to boot the firmware here,
    // but for some reason jumping to ResetVector returns to the loader.
    // If we have the control pins connected, this won't matter. If not,
    // user won't see anything on the console.
    // We can tell that control pins are connected if the ROM connect below
    // succeeds (now we are still in flasher stub and it doesn't speak the
    // ROM protocol, so if we managed to sync, it means we have control of the
    // RST pin).

    if (flasher_client.disconnect().ok() && rom.connect().ok()) {
      return rom.rebootIntoFirmware();
    }

    return util::Status(util::error::UNAVAILABLE,
                        "Flashing succeded but reboot failed.\nYou may need to "
                        "reboot manually.");
  }

  // readFlashParamsLocked puts a snippet of code in the RAM and executes it.
  // You need to reboot the device again after that to talk to the bootloader
  // again.
  util::StatusOr<QByteArray> readFlashParamsLocked(ESPFlasherClient *fc) {
    auto res = fc->read(0, 4);
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
  util::StatusOr<QByteArray> mergeFlashLocked(ESPFlasherClient *fc) {
    emit statusMessage(tr("Reading file system image (%1 @ %2)...")
                           .arg(spiffs_size_)
                           .arg(spiffs_offset_, 0, 16),
                       true);
    connect(fc, &ESPFlasherClient::progress, [this](int bytesRead) {
      emit progress(this->progress_ + bytesRead);
    });
    auto dev_fs = fc->read(spiffs_offset_, spiffs_size_);
    disconnect(fc, &ESPFlasherClient::progress, 0, 0);
    if (!dev_fs.ok()) {
      return dev_fs.status();
    }
    progress_ += spiffs_size_;
    if (!fs_dump_filename_.isEmpty()) {
      QFile f(fs_dump_filename_);
      if (f.open(QIODevice::WriteOnly)) {
        f.write(dev_fs.ValueOrDie());
      } else {
        qCritical() << "Failed to open" << fs_dump_filename_ << ":"
                    << f.errorString();
      }
    }
    auto merged =
        mergeFilesystems(dev_fs.ValueOrDie(), images_[spiffs_offset_]);
    if (merged.ok() && !files_.empty()) {
      merged = mergeFiles(merged.ValueOrDie(), files_);
    }
    if (!merged.ok()) {
      QString msg = tr("Failed to merge file system: ") +
                    QString(merged.status().ToString().c_str()) +
                    tr("\nWhat should we do?");
      int answer =
          prompter_->Prompt(msg, {{tr("Cancel"), QMessageBox::RejectRole},
                                  {tr("Write new"), QMessageBox::YesRole},
                                  {tr("Keep old"), QMessageBox::NoRole}});
      qCritical() << msg << "->" << answer;
      switch (answer) {
        case 0:
          return merged.status();
        case 1:
          return images_[spiffs_offset_];
        case 2:
          return QByteArray();
      }
    }
    return merged;
  }

  util::StatusOr<bool> findIdLocked(ESPFlasherClient *fc) {
    // Block with ID has the following structure:
    // 1) 20-byte SHA-1 hash of the payload
    // 2) payload (JSON object)
    // 3) 1-byte terminator ('\0')
    // 4) padding with 0xFF bytes to the block size
    auto res = fc->read(idBlockOffset, idBlockSize);
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

  QMap<ulong, QByteArray> dedupImages(ESPFlasherClient *fc,
                                      QMap<ulong, QByteArray> images) {
    QMap<ulong, QByteArray> result;
    emit statusMessage("Checking existing contents...", true);
    for (auto im = images.constBegin(); im != images.constEnd(); im++) {
      const ulong addr = im.key();
      const QByteArray &data = im.value();
      emit statusMessage(
          tr("Checksumming %1 @ 0x%2...").arg(data.length()).arg(addr, 0, 16),
          true);
      ESPFlasherClient::DigestResult digests;
      auto dr = fc->digest(addr, data.length(), flashBlockSize);
      QMap<ulong, QByteArray> newImages;
      if (!dr.ok()) {
        qWarning() << "Error computing digest:" << dr.status();
        return images;
      }
      digests = dr.ValueOrDie();
      int numBlocks = (data.length() + flashBlockSize - 1) / flashBlockSize;
      quint32 newAddr = addr, newLen = 0;
      quint32 newImageSize = 0;
      for (int i = 0; i < numBlocks; i++) {
        int offset = i * flashBlockSize;
        int len = flashBlockSize;
        if (len > data.length() - offset) len = data.length() - offset;
        const QByteArray &hash = QCryptographicHash::hash(
            data.mid(offset, len), QCryptographicHash::Md5);
        qDebug() << i << offset << len << hash.toHex()
                 << digests.blockDigests[i].toHex();
        if (hash == digests.blockDigests[i]) {
          // This block is the same, skip it. Flush previous image, if any.
          if (newLen > 0) {
            result[newAddr] = data.mid(newAddr - addr, newLen);
            newLen = 0;
            qDebug() << "New image:" << result[newAddr].length() << "@" << hex
                     << showbase << newAddr;
          }
          progress_ += len;
          emit progress(progress_);
        } else {
          // This block is different. Start new or extend existing image.
          if (newLen == 0) {
            newAddr = addr + i * flashBlockSize;
          }
          newLen += len;
          newImageSize += len;
        }
      }
      if (newLen > 0) {
        result[newAddr] = data.mid(newAddr - addr, newLen);
        qDebug() << "New image:" << result[newAddr].length() << "@" << hex
                 << showbase << newAddr;
      }
      qInfo() << hex << showbase << addr << "was" << dec << data.length()
              << "now" << newImageSize;
    }
    qDebug() << "After deduping:" << result.size() << "images";
    return result;
  }

  util::Status verifyImages(ESPFlasherClient *fc,
                            QMap<ulong, QByteArray> images) {
    for (auto im = images.constBegin(); im != images.constEnd(); im++) {
      const ulong addr = im.key();
      const QByteArray &data = im.value();
      emit statusMessage(tr("Verifying image at 0x%1...").arg(addr, 0, 16),
                         true);
      auto dr = fc->digest(addr, data.length(), 0 /* no block sums */);
      if (!dr.ok()) {
        return QSP(tr("failed to compute digest of 0x%1").arg(addr, 0, 16),
                   dr.status());
      }
      ESPFlasherClient::DigestResult digests = dr.ValueOrDie();
      const QByteArray &hash =
          QCryptographicHash::hash(data, QCryptographicHash::Md5);
      qDebug() << hex << showbase << addr << data.length() << hash.toHex()
               << digests.digest.toHex();
      if (hash != digests.digest) {
        return QS(util::error::DATA_LOSS,
                  tr("Hash mismatch for image 0x%1").arg(addr, 0, 16));
      }
    }
    return util::Status::OK;
  }

  Prompter *prompter_;

  mutable QMutex lock_;
  QMap<ulong, QByteArray> images_;
  QMap<QString, QByteArray> files_;
  QSerialPort *port_;
  std::unique_ptr<ESPROMClient> rom_;
  int progress_ = 0;
  bool preserve_flash_params_ = true;
  qint32 override_flash_params_ = -1;
  bool merge_flash_filesystem_ = false;
  bool generate_id_if_none_found_ = true;
  QString id_hostname_;
  QString flashing_port_name_;
  int flashing_speed_ = kDefaultFlashBaudRate;
  bool minimize_writes_ = true;
  ulong spiffs_size_ = 0;
  ulong spiffs_offset_ = 0;
  QString fs_dump_filename_;
};

class ESP8266HAL : public HAL {
  util::Status probe(const QSerialPortInfo &port) const override {
    auto r = connectSerial(port, 9600);
    if (!r.ok()) {
      return r.status();
    }
    std::unique_ptr<QSerialPort> s(r.ValueOrDie());

    ESPROMClient rom(s.get(), s.get());

    if (!rom.connect().ok()) {
      return util::Status(util::error::UNAVAILABLE,
                          "Failed to reboot into bootloader");
    }

    auto mac = rom.readMAC();
    if (!mac.ok()) {
      qDebug() << "Error reading MAC address:" << mac.status();
      return mac.status();
    }
    qInfo() << "MAC address: " << mac.ValueOrDie().toHex();

    rom.softReset();

    return util::Status::OK;
  }

  std::unique_ptr<Flasher> flasher(Prompter *prompter) const override {
    return std::move(std::unique_ptr<Flasher>(new FlasherImpl(prompter)));
  }

  std::string name() const override {
    return "ESP8266";
  }

  util::Status reboot(QSerialPort *port) const override {
    // TODO(rojer): Bring flashing data port setting here somehow.
    ESPROMClient rom(port, port);
    // To make sure we actually control things, connect to ROM first.
    util::Status st = rom.connect();
    if (!st.ok()) return QSP("failed to communicate to ROM", st);
    return rom.rebootIntoFirmware();
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

util::StatusOr<int> flashParamsFromString(const QString &s) {
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

void addOptions(Config *config) {
  // QCommandLineOption supports C++11-style initialization only since Qt 5.4.
  QList<QCommandLineOption> opts;
  opts.append(QCommandLineOption(
      kFlashParamsOption,
      "Override params bytes read from existing firmware. Either a "
      "comma-separated string or a number. First component of the string is "
      "the flash mode, must be one of: qio (default), qout, dio, dout. "
      "Second component is flash size, value values: 2m, 4m (default), 8m, "
      "16m, 32m, 16m-c1, 32m-c1, 32m-c2. Third one is flash frequency, valid "
      "values: 40m (default), 26m, 20m, 80m. If it's a number, only 2 lowest "
      "bytes from it will be written in the header of section 0x0000 in "
      "big-endian byte order (i.e. high byte is put at offset 2, low byte at "
      "offset 3).",
      "params"));
  opts.append(QCommandLineOption(
      kSkipReadingFlashParamsOption,
      "If set and --esp8266-flash-params is not used, reading flash params "
      "from the device will not be attempted and image at 0x0000 will be "
      "written as is."));
  opts.append(QCommandLineOption(
      kFlashingDataPortOption,
      "If set, communication with ROM will be performed using another serial "
      "port. DTR/RTS signals for rebooting and console will still use the "
      "main port.",
      "port"));
  opts.append(QCommandLineOption(
      kSPIFFSOffsetOption, "Location of the SPIFFS filesystem block in flash.",
      "offset", kDefaultSPIFFSOffset));
  opts.append(QCommandLineOption(kSPIFFSSizeOption,
                                 "Size of the SPIFFS region in flash.", "size",
                                 kDefaultSPIFFSSize));
  opts.append(QCommandLineOption(
      kNoMinimizeWritesOption,
      "If set, no attempt will be made to minimize the number of blocks to "
      "write by comparing current contents with the images being written."));
  config->addOptions(opts);
}

QByteArray makeIDBlock(const QString &domain) {
  QByteArray data = randomDeviceID(domain);
  QByteArray r = QCryptographicHash::hash(data, QCryptographicHash::Sha1)
                     .append(data)
                     .append("\0", 1);
  r.append(QByteArray("\xFF").repeated(idBlockSize - r.length()));
  return r;
}

}  // namespace ESP8266

#include "esp8266.moc"
