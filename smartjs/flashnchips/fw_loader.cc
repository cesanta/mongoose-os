#include "fw_loader.h"

#include <QDir>

namespace {

class DumbLoader : public FirmwareLoader {
 public:
  DumbLoader(){};
  QList<FirmwareInfo> list(const QString &dir) const override {
    QDir d(dir);
    QList<FirmwareInfo> r;
    d.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &s : d.entryList()) {
      r.push_back(FirmwareInfo(s, "", d.absoluteFilePath(s)));
    }
    return r;
  }
};

}  // namespace

std::unique_ptr<FirmwareLoader> dirListingLoader() {
  return std::unique_ptr<FirmwareLoader>(new DumbLoader);
}
