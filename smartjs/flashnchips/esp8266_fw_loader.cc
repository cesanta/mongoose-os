#include "esp8266_fw_loader.h"

#include <vector>

#include <QDebug>
#include <QDir>

#include <common/util/error_codes.h>
#include <common/util/status.h>

#include "status_qt.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qInfo qWarning
#endif

namespace ESP8266 {

namespace {

const char fwFileGlob[] = "0x*.bin";

class OldFirmwareImage : public FirmwareImage {
 public:
  OldFirmwareImage(const QMap<ulong, QByteArray> &blobs,
                   const QMap<QString, QByteArray> &files)
      : blobs_(blobs), files_(files){};

  QMap<ulong, QByteArray> blobs() const override {
    return blobs_;
  }

  QMap<QString, QByteArray> files() const override {
    return files_;
  }

  static util::StatusOr<std::unique_ptr<FirmwareImage>> load(
      const QString &location) {
    QMap<ulong, QByteArray> blobs;
    QDir dir(location, fwFileGlob, QDir::Name,
             QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    if (!dir.exists()) {
      return QS(util::error::FAILED_PRECONDITION, "Directory does not exist");
    }

    const auto files = dir.entryInfoList();
    if (files.length() == 0) {
      return QS(util::error::FAILED_PRECONDITION, "No files to flash");
    }
    for (const auto &file : files) {
      qInfo() << "Loading" << file.fileName();
      bool ok = false;
      ulong addr = file.baseName().toULong(&ok, 16);
      if (!ok) {
        return QS(util::error::INVALID_ARGUMENT,
                  QString("%1 is not a valid address").arg(file.baseName()));
      }
      QFile f(file.absoluteFilePath());
      if (!f.open(QIODevice::ReadOnly)) {
        return QS(util::error::ABORTED,
                  QString("Failed to open %1").arg(file.absoluteFilePath()));
      }
      auto bytes = f.readAll();
      if (bytes.length() != file.size()) {
        blobs.clear();
        return QS(util::error::UNAVAILABLE,
                  QString("%1 has size %2, but readAll returned %3 bytes")
                      .arg(file.fileName())
                      .arg(file.size())
                      .arg(bytes.length()));
      }
      blobs[addr] = bytes;
    }

    QMap<QString, QByteArray> fs;
    if (dir.exists("fs")) {
      QDir files_dir(dir.filePath("fs"), "", QDir::Name,
                     QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
      for (const auto &file : files_dir.entryInfoList()) {
        qInfo() << "Loading" << file.fileName();
        QFile f(file.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) {
          return QS(util::error::ABORTED,
                    QString("Failed to open %1").arg(file.absoluteFilePath()));
        }
        fs.insert(file.fileName(), f.readAll());
        f.close();
      }
    }
    return std::unique_ptr<FirmwareImage>(new OldFirmwareImage(blobs, fs));
  }

  static util::StatusOr<FirmwareInfo> info(const QString &location) {
    return FirmwareInfo(QDir(location).dirName(), "", location);
  }

 private:
  QMap<ulong, QByteArray> blobs_;
  QMap<QString, QByteArray> files_;
};

}  // namespace

QList<FirmwareInfo> FirmwareLoader::list(const QString &dir) const {
  QDir d(dir);
  QList<FirmwareInfo> r;
  // TODO(imax): also check .zip
  d.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QString &s : d.entryList()) {
    r.push_back(FirmwareInfo(s, "", d.absoluteFilePath(s)));
  }
  return r;
}

util::StatusOr<std::unique_ptr<FirmwareImage>> FirmwareLoader::load(
    const QString &location) const {
  qInfo() << "Loading firmware from" << location;
  const std::vector<std::function<
      util::StatusOr<std::unique_ptr<FirmwareImage>>(const QString &) >>
      loaders = {OldFirmwareImage::load};
  for (size_t i = 0; i < loaders.size(); i++) {
    auto r = loaders[i](location);
    if (r.ok()) {
      qInfo() << "Loader" << i << "succeeded";
      return r.MoveValueOrDie();
    } else {
      qInfo() << "Loader" << i
              << "failed:" << r.status().error_message().c_str();
    }
  }
  return QS(util::error::INVALID_ARGUMENT, "Unrecognized firmware image");
}

}  // namespace ESP8266
