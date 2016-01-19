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
#include "esp8266_fw_loader.h"
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
const char kFlashingDataPortOption[] = "esp8266-flashing-data-port";
const char kSPIFFSOffsetOption[] = "esp8266-spiffs-offset";
const char kDefaultSPIFFSOffset[] = "0xec000";
const char kSPIFFSSizeOption[] = "esp8266-spiffs-size";
const char kDefaultSPIFFSSize[] = "65536";
const char kNoMinimizeWritesOption[] = "esp8266-no-minimize-writes";

const int kDefaultFlashBaudRate = 230400;
const ulong idBlockOffset = 0x10000;
const ulong idBlockSizeSectors = 1;
/* Last 16K of flash are reserved for system params. */
const quint32 kSystemParamsAreaSize = 16 * 1024;

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

    QStringList boolOpts(
        {kMergeFSOption, kNoMinimizeWritesOption, kSkipIdGenerationOption});
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
    auto r = fw_loader_.load(path);
    if (!r.ok()) {
      return r.status();
    }
    fw_ = r.MoveValueOrDie();
    return util::Status::OK;
  }

  util::Status setPort(QSerialPort *port) override {
    QMutexLocker lock(&lock_);
    port_ = port;
    return util::Status::OK;
  }

  int totalBytes() const override {
    QMutexLocker lock(&lock_);
    if (fw_ == nullptr) {
      return 0;
    }
    int r = 0;
    for (const auto &bytes : fw_->blobs().values()) {
      r += bytes.length();
    }
    // Add FS once again for reading.
    if (merge_flash_filesystem_ && fw_->blobs().contains(spiffs_offset_)) {
      r += fw_->blobs()[spiffs_offset_].length();
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
    if (fw_ == nullptr) {
      return QS(util::error::FAILED_PRECONDITION, tr("No firmware loaded"));
    }
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

    auto flashChipIDRes = flasher_client.getFlashChipID();
    quint32 flashSize = 524288;  // A safe default.
    if (flashChipIDRes.ok()) {
      quint32 mfg = (flashChipIDRes.ValueOrDie() & 0xff000000) >> 24;
      quint32 type = (flashChipIDRes.ValueOrDie() & 0x00ff0000) >> 16;
      quint32 capacity = (flashChipIDRes.ValueOrDie() & 0x0000ff00) >> 8;
      qInfo() << "Flash chip ID:" << hex << showbase << mfg << type << capacity;
      if (mfg != 0 && capacity >= 0x13 && capacity < 0x20) {
        // Capacity is the power of two.
        flashSize = 1 << capacity;
      }
    } else {
      qCritical() << "Failed to get flash chip id:" << flashChipIDRes.status();
    }
    qInfo() << "Flash size:" << flashSize;

    st = sanityCheckImages(fw_->blobs(), flashSize,
                           flasher_client.kFlashSectorSize);
    if (!st.ok()) return st;

    if (fw_->blobs().contains(0) && fw_->blobs()[0].length() >= 4) {
      int flashParams = 0;
      if (override_flash_params_ >= 0) {
        flashParams = override_flash_params_;
      } else {
        // We don't have constants for larger flash sizes.
        if (flashSize > 4194304) flashSize = 4194304;
        // We use detected size + DIO @ 40MHz which should be a safe default.
        // Advanced users wishing to use other modes and freqs can override.
        flashParams =
            flashParamsFromString(
                tr("dio,%1m,40m").arg(flashSize * 8 / 1048576)).ValueOrDie();
      }
      fw_->blobs()[0][2] = (flashParams >> 8) & 0xff;
      fw_->blobs()[0][3] = flashParams & 0xff;
      emit statusMessage(
          tr("Adjusting flash params in the image 0x0000 to 0x%1")
              .arg(QString(fw_->blobs()[0].mid(2, 2).toHex())),
          true);
    }

    bool id_generated = false;
    if (generate_id_if_none_found_ && !fw_->blobs().contains(idBlockOffset)) {
      auto res = findIdLocked(&flasher_client);
      if (res.ok()) {
        if (!res.ValueOrDie()) {
          emit statusMessage(tr("Generating new ID"), true);
          fw_->blobs()[idBlockOffset] = makeIDBlock(id_hostname_);
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
    if (merge_flash_filesystem_ && fw_->blobs().contains(spiffs_offset_)) {
      auto res = mergeFlashLocked(&flasher_client);
      if (res.ok()) {
        if (res.ValueOrDie().size() > 0) {
          fw_->blobs()[spiffs_offset_] = res.ValueOrDie();
        } else {
          fw_->blobs().remove(spiffs_offset_);
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

    auto flashImages = minimize_writes_
                           ? dedupImages(&flasher_client, fw_->blobs())
                           : fw_->blobs();

    for (ulong image_addr : flashImages.keys()) {
      QByteArray data = flashImages[image_addr];
      emit progress(progress_);
      int origLength = data.length();

      if (data.length() % flasher_client.kFlashSectorSize != 0) {
        quint32 padLen = flasher_client.kFlashSectorSize -
                         (data.length() % flasher_client.kFlashSectorSize);
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

    st = verifyImages(&flasher_client, fw_->blobs());
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

  util::Status sanityCheckImages(const QMap<ulong, QByteArray> &images,
                                 quint32 flashSize, quint32 flashSectorSize) {
    const auto keys = images.keys();
    for (int i = 0; i < keys.length() - 1; i++) {
      const quint32 imageBegin = keys[i];
      const QByteArray &image = images[imageBegin];
      const quint32 imageEnd = imageBegin + image.length();
      if (imageBegin >= flashSize || imageEnd > flashSize) {
        return QS(util::error::INVALID_ARGUMENT,
                  tr("Image %1 @ 0x%2 will not fit in flash (size %3)")
                      .arg(image.length())
                      .arg(imageBegin, 0, 16)
                      .arg(flashSize));
      }
      if (imageBegin % flashSectorSize != 0) {
        return QS(util::error::INVALID_ARGUMENT,
                  tr("Image starting address (0x%1) is not on flash sector "
                     "boundary (sector size %2)")
                      .arg(imageBegin, 0, 16)
                      .arg(flashSectorSize));
      }
      if (imageBegin == 0 && image.length() >= 1) {
        if (image[0] != (char) 0xE9) {
          return QS(util::error::INVALID_ARGUMENT,
                    tr("Invalid magic byte in the first image"));
        }
      }
      const quint32 systemParamsBegin = flashSize - kSystemParamsAreaSize;
      const quint32 systemParamsEnd = flashSize;
      if (imageBegin < systemParamsEnd && imageEnd > systemParamsBegin) {
        return QS(util::error::INVALID_ARGUMENT,
                  tr("Image 0x%1 overlaps with system params area (%2 @ 0x%3)")
                      .arg(imageBegin, 0, 16)
                      .arg(kSystemParamsAreaSize)
                      .arg(systemParamsBegin, 0, 16));
      }
      if (i > 0) {
        const quint32 prevImageBegin = keys[i - 1];
        const quint32 prevImageEnd = keys[i - 1] + images[keys[i - 1]].length();
        // We traverse the list in order, so a simple check will suffice.
        if (prevImageEnd > imageBegin) {
          return QS(util::error::INVALID_ARGUMENT,
                    tr("Images at offsets 0x%1 and 0x%2 overlap.")
                        .arg(prevImageBegin, 0, 16)
                        .arg(imageBegin, 0, 16));
        }
      }
    }
    return util::Status::OK;
  }

  // mergeFlashLocked reads the spiffs filesystem from the device
  // and mounts it in memory. Then it overwrites the files that are
  // present in the software update but it leaves the existing ones.
  // The idea is that the filesystem is mostly managed by the user
  // or by the software update utility, while the core system uploaded by
  // the flasher should only upload a few core files.
  util::StatusOr<QByteArray> mergeFlashLocked(ESPFlasherClient *fc) {
    if (fw_ == nullptr) {
      return QS(util::error::FAILED_PRECONDITION, tr("No firmware loaded"));
    }
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
        mergeFilesystems(dev_fs.ValueOrDie(), fw_->blobs()[spiffs_offset_]);
    if (merged.ok() && !fw_->files().empty()) {
      merged = mergeFiles(merged.ValueOrDie(), fw_->files());
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
          return fw_->blobs()[spiffs_offset_];
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
    auto res =
        fc->read(idBlockOffset, idBlockSizeSectors * fc->kFlashSectorSize);
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
      auto dr = fc->digest(addr, data.length(), fc->kFlashSectorSize);
      QMap<ulong, QByteArray> newImages;
      if (!dr.ok()) {
        qWarning() << "Error computing digest:" << dr.status();
        return images;
      }
      digests = dr.ValueOrDie();
      int numBlocks =
          (data.length() + fc->kFlashSectorSize - 1) / fc->kFlashSectorSize;
      quint32 newAddr = addr, newLen = 0;
      quint32 newImageSize = 0;
      for (int i = 0; i < numBlocks; i++) {
        int offset = i * fc->kFlashSectorSize;
        int len = fc->kFlashSectorSize;
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
            newAddr = addr + i * fc->kFlashSectorSize;
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
  FirmwareLoader fw_loader_;
  std::unique_ptr<FirmwareImage> fw_;
  QSerialPort *port_;
  std::unique_ptr<ESPROMClient> rom_;
  int progress_ = 0;
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

  std::unique_ptr<::FirmwareLoader> fwLoader() const override {
    return dirListingLoader();
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
  return r;
}

}  // namespace ESP8266

#include "esp8266.moc"
