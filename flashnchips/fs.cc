#include "fs.h"

#include <memory>

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include <assert.h>

#include <common/util/error_codes.h>
#include <common/util/statusor.h>

namespace {

// The SPIFFS C library requires us to pass 3 C function callbacks
// but they don't pass any custom "context". This means that we
// either have to generate C trampolines (i.e. a form of runtime
// code generation) or we just make sure only one SPIFFS FS can be
// mounted at a time.
QByteArray *mounted_image = nullptr;
QBasicMutex mounted_image_lock;

int32_t mem_spiffs_read(uint32_t addr, uint32_t size, u8_t *dst) {
  assert(mounted_image != nullptr);
  memcpy(dst, mounted_image->data() + addr, size);
  return SPIFFS_OK;
}

int32_t mem_spiffs_write(uint32_t addr, uint32_t size, u8_t *src) {
  assert(mounted_image != nullptr);
  memcpy(mounted_image->data() + addr, src, size);
  return SPIFFS_OK;
}

int32_t mem_spiffs_erase(uint32_t addr, uint32_t size) {
  assert(mounted_image != nullptr);
  memset(mounted_image->data() + addr, 0xff, size);
  return SPIFFS_OK;
}
}

class Mounter {
 public:
  Mounter(SPIFFS *fs) : fs_(fs) {
    status_ = fs_->mount();
  }

  ~Mounter() {
    if (status_.ok()) {
      fs_->unmount();
    }
  }

  util::Status status() const {
    return status_;
  }

 private:
  util::Status status_;
  SPIFFS *fs_;
};

SPIFFS::SPIFFS(QByteArray image) : image_(image) {
}

util::Status SPIFFS::mount() {
  QMutexLocker lock(&mounted_image_lock);

  if (mounted_image != nullptr) {
    return util::Status(util::error::ABORTED,
                        "can only mount one SPIFFS fs at a time");
  }
  mounted_image = &image_;

  spiffs_config cfg;

  cfg.phys_size = image_.size();
  cfg.phys_addr = 0;

  cfg.phys_erase_block = FLASH_BLOCK_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.log_page_size = LOG_PAGE_SIZE;

  cfg.hal_read_f = mem_spiffs_read;
  cfg.hal_write_f = mem_spiffs_write;
  cfg.hal_erase_f = mem_spiffs_erase;

  if (SPIFFS_mount(&fs_, &cfg, spiffs_work_buf_, spiffs_fds_,
                   sizeof(spiffs_fds_), 0, 0, 0) == -1) {
    return util::Status(util::error::ABORTED, "SPIFFS_mount failed");
  }
  return util::Status::OK;
}

void SPIFFS::unmount() {
  QMutexLocker lock(&mounted_image_lock);
  if (mounted_image == &image_) {
    mounted_image = nullptr;
    SPIFFS_unmount(&fs_);
  } else {
    qWarning()
        << "some other instance of SPIFFS is registered as the global instance";
  }
}

util::Status SPIFFS::merge(SPIFFS &other) {
  util::StatusOr<QMap<QString, QByteArray>> files_status = other.files();
  if (!files_status.ok()) {
    return files_status.status();
  }
  auto files = files_status.ValueOrDie();
  return mergeFiles(files);
}

util::Status SPIFFS::mergeFiles(const QMap<QString, QByteArray> &files) {
  Mounter m(this);

  for (auto i = files.constBegin(); i != files.constEnd(); i++) {
    char *fname = const_cast<char *>(i.key().toStdString().c_str());
    int sfd =
        SPIFFS_open(&fs_, fname, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (sfd == -1) {
      qCritical() << "SPIFFS_open " << fname
                  << " failed: " << SPIFFS_errno(&fs_);
      SPIFFS_close(&fs_, sfd);
      if (SPIFFS_errno(&fs_) == SPIFFS_ERR_FULL) {
        return util::Status(util::error::ABORTED, "SPIFFS filesystem full");
      }
      return util::Status(util::error::ABORTED, "SPIFFS_open failed");
    }

    uint8_t *d =
        reinterpret_cast<uint8_t *>(const_cast<char *>(i.value().data()));
    if (SPIFFS_write(&fs_, sfd, d, i.value().size()) == -1) {
      qCritical() << "SPIFFS_write " << fname
                  << " failed: " << SPIFFS_errno(&fs_);
      if (SPIFFS_errno(&fs_) == SPIFFS_ERR_FULL) {
        return util::Status(util::error::ABORTED, "SPIFFS filesystem full");
      }
      return util::Status(util::error::ABORTED, "SPIFFS_write failed");
    }

    SPIFFS_close(&fs_, sfd);
  }
  return util::Status::OK;
}

util::StatusOr<QMap<QString, QByteArray>> SPIFFS::files() {
  QMap<QString, QByteArray> res;

  spiffs_DIR dh;
  struct spiffs_dirent de;
  struct spiffs_dirent *d;

  Mounter m(this);
  if (!m.status().ok()) {
    return m.status();
  }

  SPIFFS_opendir(&fs_, (char *) ".", &dh);
  while ((d = SPIFFS_readdir(&dh, &de)) != nullptr) {
    int rfd = SPIFFS_open(&fs_, (char *) d->name, SPIFFS_RDONLY, 0);
    if (rfd == -1) {
      qCritical() << "Cannot open" << (char *) d->name;
      return util::Status(util::error::ABORTED, "cannot open");
    }
    std::unique_ptr<char[]> buf(new char[image_.size()]);
    s32_t n = SPIFFS_read(&fs_, rfd, buf.get(), image_.size());
    SPIFFS_close(&fs_, rfd);
    if (n < 0) {
      qCritical() << "Failed to read" << (char *) d->name;
      return util::Status(util::error::ABORTED, "read failed");
    }
    res.insert(reinterpret_cast<char *>(d->name), QByteArray(buf.get(), n));
  }

  return res;
}

QByteArray SPIFFS::data() const {
  return image_;
}
