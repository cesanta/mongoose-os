/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp32/src/esp32_mmap.h"

/*
 * Translates vfs file descriptor from vfs to a spiffs file descriptor; also
 * returns an instance of spiffs through the pointer.
 */
int esp_translate_fd(int fd, spiffs **pfs) {
  void *ctx;
  fd = esp_vfs_translate_fd(fd, &ctx);
  *pfs = &((struct mount_info *) ctx)->fs;
  return fd;
}
