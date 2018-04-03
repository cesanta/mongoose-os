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

#include <string.h>

#include "mgos_vfs_dev.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

struct mgos_vfs_dev_entry {
  const char *type;
  const struct mgos_vfs_dev_ops *ops;
  SLIST_ENTRY(mgos_vfs_dev_entry) next;
};

static SLIST_HEAD(s_devs,
                  mgos_vfs_dev_entry) s_devs = SLIST_HEAD_INITIALIZER(s_devs);

bool mgos_vfs_dev_register_type(const char *type,
                                const struct mgos_vfs_dev_ops *ops) {
  if (ops->open == NULL || ops->read == NULL || ops->write == NULL ||
      ops->erase == NULL || ops->get_size == NULL || ops->close == NULL) {
    /* All methods must be implemented, even if with dummy functions. */
    LOG(LL_ERROR, ("%s: not all methods are implemented", type));
    abort();
  }
  struct mgos_vfs_dev_entry *de =
      (struct mgos_vfs_dev_entry *) calloc(1, sizeof(*de));
  if (de == NULL) return false;
  de->type = type;
  de->ops = ops;
  SLIST_INSERT_HEAD(&s_devs, de, next);
  return true;
}

struct mgos_vfs_dev *mgos_vfs_dev_open(const char *type, const char *opts) {
  struct mgos_vfs_dev *dev = NULL;
  struct mgos_vfs_dev_entry *de;
  SLIST_FOREACH(de, &s_devs, next) {
    if (strcmp(type, de->type) == 0) {
      if (opts == NULL) opts = "";
      dev = (struct mgos_vfs_dev *) calloc(1, sizeof(*dev));
      LOG(LL_INFO, ("%s (%s) -> %p", type, opts, dev));
      dev->ops = de->ops;
      if (!de->ops->open(dev, opts)) {
        LOG(LL_ERROR, ("Dev %s %s open failed", type, opts));
        free(dev);
        dev = NULL;
      }
      return dev;
    }
  };
  LOG(LL_ERROR, ("Unknown device type %s", type));
  return NULL;
}

bool mgos_vfs_dev_close(struct mgos_vfs_dev *dev) {
  bool ret = false;
  LOG(LL_DEBUG, ("%p refs %d", dev, dev->refs));
  if (dev->refs <= 0) {
    ret = dev->ops->close(dev);
    if (ret) free(dev);
  }
  return ret;
}
