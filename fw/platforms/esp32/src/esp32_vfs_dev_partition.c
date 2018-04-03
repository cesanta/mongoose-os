/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fw/platforms/esp32/src/esp32_vfs_dev_partition.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_partition.h"

#include "common/cs_dbg.h"

#include "frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_dev.h"

static bool esp32_vfs_dev_partition_open(struct mgos_vfs_dev *dev,
                                         const char *opts) {
  char *label = NULL;
  int subtype = 0xff; /* any subtype */
  json_scanf(opts, strlen(opts), "{name: %Q, label: %Q, subtype: %d}", &label,
             &label, &subtype);
  if (label == NULL) {
    LOG(LL_ERROR, ("Must specify partition label"));
    return false;
  }
  const esp_partition_t *part =
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, subtype, label);
  if (part != NULL) {
    dev->dev_data = (void *) part;
  }
  free(label);
  return (part != NULL);
}

static bool esp32_vfs_dev_partition_read(struct mgos_vfs_dev *dev,
                                         size_t offset, size_t len, void *dst) {
  const esp_partition_t *p = (esp_partition_t *) dev->dev_data;
  if (len > p->size || offset + len > p->address + p->size) {
    LOG(LL_ERROR, ("%s: invalid read args: %u @ %u", p->label, len, offset));
    return false;
  }
  esp_err_t r = spi_flash_read(p->address + offset, dst, len);
  LOG((r == ESP_OK ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s: %s %u @ %d = %d", p->label, "read", len, offset, r));
  return (r == ESP_OK);
}

static bool esp32_vfs_dev_partition_write(struct mgos_vfs_dev *dev,
                                          size_t offset, size_t len,
                                          const void *src) {
  const esp_partition_t *p = (esp_partition_t *) dev->dev_data;
  if (len > p->size || len + len > p->address + p->size) {
    LOG(LL_ERROR, ("%s: invalid write args: %u @ %u", p->label, len, offset));
    return false;
  }
  mgos_wdt_feed();
  esp_err_t r = spi_flash_write(p->address + offset, src, len);
  LOG((r == ESP_OK ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s: %s %u @ %d = %d", p->label, "write", len, offset, r));
  return (r == ESP_OK);
}

static bool esp32_vfs_dev_partition_erase(struct mgos_vfs_dev *dev,
                                          size_t offset, size_t len) {
  const esp_partition_t *p = (esp_partition_t *) dev->dev_data;
  if (len > p->size || offset + len > p->address + p->size ||
      offset % SPI_FLASH_SEC_SIZE != 0 || len % SPI_FLASH_SEC_SIZE != 0) {
    LOG(LL_ERROR, ("Invalid erase args: %u @ %u", len, offset));
    return false;
  }
  mgos_wdt_feed();
  esp_err_t r = spi_flash_erase_range(p->address + offset, len);
  LOG((r == ESP_OK ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%s: %s %u @ %d = %d", p->label, "erase", len, offset, r));
  return (r == ESP_OK);
}

static size_t esp32_vfs_dev_partition_get_size(struct mgos_vfs_dev *dev) {
  const esp_partition_t *p = (esp_partition_t *) dev->dev_data;
  return p->size;
}

static bool esp32_vfs_dev_partition_close(struct mgos_vfs_dev *dev) {
  /* Nothing to do. */
  (void) dev;
  return true;
}

static const struct mgos_vfs_dev_ops esp32_vfs_dev_partition_ops = {
    .open = esp32_vfs_dev_partition_open,
    .read = esp32_vfs_dev_partition_read,
    .write = esp32_vfs_dev_partition_write,
    .erase = esp32_vfs_dev_partition_erase,
    .get_size = esp32_vfs_dev_partition_get_size,
    .close = esp32_vfs_dev_partition_close,
};

bool esp32_vfs_dev_partition_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_ESP32_PARTITION,
                                    &esp32_vfs_dev_partition_ops);
}
