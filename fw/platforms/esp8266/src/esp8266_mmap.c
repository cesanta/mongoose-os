/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp8266_mmap.h"

/*
 * Translates vfs file descriptor from vfs to a spiffs file descriptor; also
 * returns an instance of spiffs through the pointer.
 */
int esp_translate_fd(int fd, spiffs **pfs) {
  *pfs = cs_spiffs_get_fs();
  return fd - NUM_SYS_FD;
}
