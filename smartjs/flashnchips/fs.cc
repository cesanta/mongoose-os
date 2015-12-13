#include "fs.h"

#include <memory>

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include <assert.h>

#include <common/util/error_codes.h>
#include <common/util/statusor.h>

namespace {

int32_t mem_spiffs_read(struct spiffs_t *fs, uint32_t addr, uint32_t size,
                        u8_t *dst) {
  SPIFFS *fsc = static_cast<SPIFFS *>(fs->user_data);
  memcpy(dst, fsc->mounted_image()->data() + addr, size);
  return SPIFFS_OK;
}

int32_t mem_spiffs_write(struct spiffs_t *fs, uint32_t addr, uint32_t size,
                         u8_t *src) {
  SPIFFS *fsc = static_cast<SPIFFS *>(fs->user_data);
  memcpy(fsc->mounted_image()->data() + addr, src, size);
  return SPIFFS_OK;
}

int32_t mem_spiffs_erase(struct spiffs_t *fs, uint32_t addr, uint32_t size) {
  SPIFFS *fsc = static_cast<SPIFFS *>(fs->user_data);
  memset(fsc->mounted_image()->data() + addr, 0xff, size);
  return SPIFFS_OK;
}

}  // namespace

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

SPIFFS::SPIFFS(int size) {
  image_.resize(size);
  for (int i = 0; i < size; i++) image_[i] = 0xff;
  mount();  // This will fail but is required per documentation.
  if (SPIFFS_format(&fs_) != SPIFFS_OK) {
    qFatal("Could not format SPIFFS (size %d): %d", size, SPIFFS_errno(&fs_));
  }
  qDebug() << "Created SPIFFS" << this << ", size" << size;
}

util::Status SPIFFS::mount() {
  spiffs_config cfg;

  fs_.user_data = this;

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
    return util::Status(
        util::error::ABORTED,
        "SPIFFS_mount failed: " + std::to_string(SPIFFS_errno(&fs_)));
  }

  qDebug() << "Mounted SPIFFS" << this << ":\n";
  SPIFFS_vis(&fs_);
  return util::Status::OK;
}

void SPIFFS::unmount() {
  SPIFFS_unmount(&fs_);
}

util::StatusOr<std::map<QString, QByteArray>> SPIFFS::files() {
  std::map<QString, QByteArray> res;

  spiffs_DIR dh;
  struct spiffs_dirent de;
  struct spiffs_dirent *d;

  Mounter m(this);
  if (!m.status().ok()) {
    return m.status();
  }

  qDebug() << "Listing files in" << this;
  SPIFFS_opendir(&fs_, (char *) ".", &dh);
  while ((d = SPIFFS_readdir(&dh, &de)) != nullptr) {
    QString name((const char *) d->name);
    qDebug() << name << d->size << "bytes";
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
    res[name] = QByteArray(buf.get(), n);
  }

  return res;
}

QByteArray SPIFFS::image() const {
  return image_;
}

spiffs *SPIFFS::fs() {
  return &fs_;
}

util::StatusOr<QByteArray> mergeFiles(QByteArray old_fs_image,
                                      QMap<QString, QByteArray> new_files) {
  if (old_fs_image.isEmpty() && new_files.empty()) return QByteArray();
  std::map<QString, QByteArray> files;
  if (!old_fs_image.isEmpty()) {
    SPIFFS old_fs(old_fs_image);
    auto old_files = old_fs.files();
    if (!old_files.ok()) {
      return util::Status(util::error::ABORTED,
                          "Unable to read device file system: " +
                              old_files.status().ToString());
    }
    files.insert(old_files.ValueOrDie().begin(), old_files.ValueOrDie().end());
  }
  {
    std::map<QString, QByteArray> new_files_sm(new_files.toStdMap());
    for (const auto &f : new_files_sm) {
      files[f.first] = f.second;
    }
    // There is currently no way to delete files.
  }
  {
    SPIFFS merged_fs(old_fs_image.size());
    Mounter m(&merged_fs);
    for (const auto f : files) {
      std::string fname = f.first.toStdString();
      qDebug("Writing '%s' (%d bytes)", fname.c_str(),
             static_cast<int>(f.second.size()));
      int sfd = SPIFFS_open(merged_fs.fs(), const_cast<char *>(fname.c_str()),
                            SPIFFS_CREAT | SPIFFS_RDWR, 0);
      if (sfd < 0) {
        qCritical() << "SPIFFS_open " << f.first
                    << " failed: " << SPIFFS_errno(merged_fs.fs());
        SPIFFS_close(merged_fs.fs(), sfd);
        if (SPIFFS_errno(merged_fs.fs()) == SPIFFS_ERR_FULL) {
          return util::Status(util::error::ABORTED, "SPIFFS filesystem full");
        }
        return util::Status(util::error::ABORTED,
                            "SPIFFS_open '" + f.first.toStdString() +
                                "' failed: " +
                                std::to_string(SPIFFS_errno(merged_fs.fs())));
      }

      uint8_t *d =
          reinterpret_cast<uint8_t *>(const_cast<char *>(f.second.data()));
      if (SPIFFS_write(merged_fs.fs(), sfd, d, f.second.size()) == -1) {
        qCritical() << "SPIFFS_write '" << f.first << "' (" << f.second.size()
                    << ") failed: " << SPIFFS_errno(merged_fs.fs());
        if (SPIFFS_errno(merged_fs.fs()) == SPIFFS_ERR_FULL) {
          SPIFFS_vis(merged_fs.fs());
          return util::Status(util::error::ABORTED, "SPIFFS filesystem full");
        }
        return util::Status(util::error::ABORTED, "SPIFFS_write failed");
      }

      SPIFFS_close(merged_fs.fs(), sfd);
    }
    return std::move(merged_fs.image());
  }
}

util::StatusOr<QByteArray> mergeFilesystems(QByteArray old_fs_image,
                                            QByteArray new_fs_image) {
  QMap<QString, QByteArray> new_files;
  if (!new_fs_image.isEmpty()) {
    SPIFFS new_fs(new_fs_image);
    auto new_files_st = new_fs.files();
    if (!new_files_st.ok()) {
      return util::Status(util::error::ABORTED,
                          "Unable to read new file system: " +
                              new_files_st.status().ToString());
    }
    new_files = QMap<QString, QByteArray>(new_files_st.ValueOrDie());
  }
  return mergeFiles(old_fs_image, new_files);
}
