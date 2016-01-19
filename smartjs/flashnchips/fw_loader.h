#ifndef FW_LOADER_H
#define FW_LOADER_H

#include <memory>

#include <QList>
#include <QString>

class FirmwareInfo {
 public:
  FirmwareInfo(const QString &name, const QString &description,
               const QString &location)
      : name_(name), description_(description), location_(location){};
  QString name() const {
    return name_;
  }
  QString description() const {
    return description_;
  }
  QString location() const {
    return location_;
  }

 private:
  QString name_;
  QString description_;
  QString location_;
};

class FirmwareLoader {
 public:
  virtual ~FirmwareLoader(){};
  virtual QList<FirmwareInfo> list(const QString &dir) const = 0;
};

// dirListingLoader returns a FirmwareLoader that simply lists subdirectories
// when queried, which is the old behaviour of FlashNChips UI.
std::unique_ptr<FirmwareLoader> dirListingLoader();

#endif  // FW_LOADER_H
