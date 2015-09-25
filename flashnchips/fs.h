#ifndef FS_H
#define FS_H

#include <QMap>
#include <QByteArray>
#include <QString>

#include <spiffs.h>

#include <common/util/statusor.h>

// conf taken from ESP8266 smartjs config
#define LOG_PAGE_SIZE 256
#define FLASH_BLOCK_SIZE (4 * 1024)

// in memory spiffs implementation
class SPIFFS {
  friend class Mounter;

 public:
  SPIFFS(QByteArray image);
  util::Status merge(SPIFFS& other);
  util::Status mergeFiles(const QMap<QString, QByteArray>& files);
  QByteArray data() const;

 protected:
  util::Status mount();
  void unmount();

 private:
  SPIFFS(const SPIFFS&) = delete;
  SPIFFS& operator=(const SPIFFS&) = delete;

  util::StatusOr<QMap<QString, QByteArray>> files();

  QByteArray image_;
  spiffs fs_;

  // conf taken from ESP8266 smartjs config
  uint8_t spiffs_work_buf_[LOG_PAGE_SIZE * 2];
  uint8_t spiffs_fds_[32 * 4];
};

#endif  // FS_H
