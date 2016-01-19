#ifndef ESP8266_FW_LOADER_H
#define ESP8266_FW_LOADER_H

#include <memory>

#include <QByteArray>
#include <QMap>
#include <QString>

#include <common/util/statusor.h>

#include "fw_loader.h"

namespace ESP8266 {

class FirmwareImage {
 public:
  virtual ~FirmwareImage(){};
  virtual QMap<ulong, QByteArray> blobs() const = 0;
  virtual QMap<QString, QByteArray> files() const {
    return QMap<QString, QByteArray>();
  };
  virtual bool isOTAWithRBoot() const {
    return false;
  }
};

class FirmwareLoader : public ::FirmwareLoader {
 public:
  QList<FirmwareInfo> list(const QString &dir) const override;
  util::StatusOr<std::unique_ptr<FirmwareImage>> load(
      const QString &location) const;
};

}  // namespace ESP8266

#endif  // ESP8266_FW_LOADER_H
