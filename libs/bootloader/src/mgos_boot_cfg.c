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

#include "mgos_boot_cfg.h"

#include <string.h>

#include "common/cs_crc32.h"
#include "common/cs_dbg.h"

#include "mgos_vfs_dev.h"

#include "mgos_boot_dbg.h"

#if defined(MGOS_BL_ENABLE) || defined(MGOS_BOOT_BUILD) /* ifdef-ok */
struct mgos_boot_state g_boot_state __attribute((section(".boot_state")));

static struct mgos_vfs_dev *s_bcfg0_dev, *s_bcfg1_dev;

static struct mgos_vfs_dev *s_bcfg_dev;
static size_t s_bcfg_off;

static struct mgos_boot_cfg *s_bcfg;

static void mgos_boot_cfg_find_latest_dev(struct mgos_vfs_dev *dev, bool *found,
                                          struct mgos_boot_cfg *cfg,
                                          struct mgos_vfs_dev **cfg_dev,
                                          size_t *cfg_off) {
  struct mgos_boot_cfg_record cfgr;
  size_t dev_size = mgos_vfs_dev_get_size(dev);
  for (size_t off = 0; off < dev_size; off += sizeof(cfgr)) {
    if (mgos_vfs_dev_read(dev, off, sizeof(cfgr), &cfgr) != 0) break;
    if (cfgr.cfg.magic != MGOS_BOOT_CFG_MAGIC) break;
    uint32_t crc32 = cs_crc32(0, &cfgr, sizeof(cfgr) - sizeof(uint32_t));
    if (crc32 != cfgr.crc32) break;
    if (*found && cfgr.cfg.seq <= cfg->seq) break;
    *cfg_dev = dev;
    *cfg_off = off;
    *cfg = cfgr.cfg;
    *found = true;
    strcpy(g_boot_state.cfg_dev, dev->name);
    g_boot_state.cfg_off = off;
    memcpy(&g_boot_state.cfgr, &cfgr, sizeof(g_boot_state.cfgr));
  }
}

static bool mgos_boot_cfg_write_dev(const struct mgos_boot_cfg *cfg,
                                    struct mgos_vfs_dev *dev, size_t off,
                                    bool dump) {
  const char *what;
  enum mgos_vfs_dev_err r;
  struct mgos_boot_cfg_record cfgr;
  uint32_t crc1;
  memset(&cfgr, 0xff, sizeof(cfgr));
  memcpy(&cfgr.cfg, cfg, sizeof(cfgr.cfg));
  cfgr.crc32 = crc1 = cs_crc32(0, &cfgr, sizeof(cfgr) - sizeof(uint32_t));
  if (off == 0) mgos_vfs_dev_erase(dev, 0, mgos_vfs_dev_get_size(dev));
  r = mgos_vfs_dev_write(dev, off, sizeof(cfgr), &cfgr);
  if (r != 0) {
    what = "write";
    goto out;
  }
  r = mgos_vfs_dev_read(dev, off, sizeof(cfgr), &cfgr);
  if (r != 0) {
    what = "read";
    goto out;
  }
  uint32_t crc2 = cs_crc32(0, &cfgr, sizeof(cfgr) - sizeof(uint32_t));
  if (crc1 != crc2) {
    what = "verify";
    r = MGOS_VFS_DEV_ERR_CORRUPT;
  }
  s_bcfg_dev = dev;
  s_bcfg_off = off;
  what = "write";
out:
  if (r == 0) {
    strcpy(g_boot_state.cfg_dev, dev->name);
    g_boot_state.cfg_off = off;
    memcpy(&g_boot_state.cfgr, &cfgr, sizeof(g_boot_state.cfgr));
    if (dump) {
      mgos_boot_cfg_dump(cfg);
    } else {
      mgos_boot_dbg_printf(
          "Cfg seq %lu %s %s (%d) @ %s:%lu\n", (unsigned long) cfg->seq, what,
          (r == 0 ? "ok" : "failed"), r, dev->name, (unsigned long) off);
    }
  }
  return r == 0;
}

bool mgos_boot_cfg_write(struct mgos_boot_cfg *cfg, bool dump) {
  struct mgos_vfs_dev *dev = s_bcfg_dev;
  size_t off;
  cfg->seq++;
  if (s_bcfg_dev == NULL) {
    dev = s_bcfg0_dev;
    off = 0;
  } else {
    off = s_bcfg_off + MGOS_BOOT_CFG_REC_SIZE;
  }
  if (off + MGOS_BOOT_CFG_REC_SIZE > mgos_vfs_dev_get_size(dev) ||
      off >= MGOS_BOOT_CFG_MAX_RECS_PER_DEV * MGOS_BOOT_CFG_REC_SIZE) {
    /* Time to switch devices. */
    dev =
        (dev == s_bcfg0_dev && s_bcfg1_dev != NULL ? s_bcfg1_dev : s_bcfg0_dev);
    off = 0;
  }
  bool res = mgos_boot_cfg_write_dev(cfg, dev, off, dump);
  if (!res) {
    /* Didn't work? Try switching devices with erase. */
    dev =
        (dev == s_bcfg0_dev && s_bcfg1_dev != NULL ? s_bcfg1_dev : s_bcfg0_dev);
    res = mgos_boot_cfg_write_dev(cfg, dev, 0, dump);
  }
  return res;
}

void mgos_boot_cfg_dump(const struct mgos_boot_cfg *cfg) {
  mgos_boot_dbg_printf("Cfg seq %lu nsl %d a %d r %d f 0x%lx(%c%c%c%c)\n",
                       (unsigned long) cfg->seq, cfg->num_slots,
                       cfg->active_slot, cfg->revert_slot,
                       (unsigned long) cfg->flags,
                       (cfg->flags & MGOS_BOOT_F_MERGE_FS ? 'M' : '.'),
                       (cfg->flags & MGOS_BOOT_F_FIRST_BOOT_B ? 'F' : '.'),
                       (cfg->flags & MGOS_BOOT_F_FIRST_BOOT_A ? 'f' : '.'),
                       (cfg->flags & MGOS_BOOT_F_COMMITTED ? 'C' : '.'));
  for (int i = 0; i < cfg->num_slots; i++) {
    const struct mgos_boot_slot_cfg *sc = &cfg->slots[i].cfg;
    const struct mgos_boot_slot_state *ss = &cfg->slots[i].state;
    mgos_boot_dbg_printf(
        "%d: 0x%lx(%c%c) %s ma 0x%lx fs %s; "
        "%lu org 0x%lx crc 0x%lx f 0x%lx e %u\n",
        i, (unsigned long) sc->flags,
        (sc->flags & MGOS_BOOT_SLOT_F_WRITEABLE ? 'W' : '.'),
        (sc->flags & MGOS_BOOT_SLOT_F_VALID ? 'V' : '.'), sc->app_dev,
        (unsigned long) sc->app_map_addr,
        (sc->fs_dev[0] != '\0' ? sc->fs_dev : "-"), (unsigned long) ss->app_len,
        (unsigned long) ss->app_org, (unsigned long) ss->app_crc32,
        (unsigned long) ss->app_flags, ss->err_count);
  }
}

struct mgos_boot_cfg *mgos_boot_cfg_get(void) {
  return s_bcfg;
}

bool mgos_boot_cfg_find_latest(void) {
  bool found = false;
  mgos_boot_cfg_find_latest_dev(s_bcfg0_dev, &found, s_bcfg, &s_bcfg_dev,
                                &s_bcfg_off);
  mgos_boot_cfg_find_latest_dev(s_bcfg1_dev, &found, s_bcfg, &s_bcfg_dev,
                                &s_bcfg_off);
  return found;
}

void mgos_boot_cfg_set_default(struct mgos_boot_cfg *cfg) {
  memset(cfg, 0, sizeof(*cfg));
  cfg->magic = MGOS_BOOT_CFG_MAGIC;
  cfg->version = 1;
  cfg->flags = MGOS_BOOT_F_COMMITTED;
  cfg->revert_slot = -1;
  mgos_boot_cfg_set_default_slots(cfg);
}

int8_t mgos_boot_cfg_find_slot(const struct mgos_boot_cfg *cfg,
                               uintptr_t app_map_addr, bool want_fs,
                               int8_t excl1, int8_t excl2) {
  int8_t res = -1;
  /* We randomize somewhat by starting at different point. */
  for (uint32_t j = 0; j < cfg->num_slots; j++) {
    int8_t i = (int8_t)((cfg->seq + j) % cfg->num_slots);
    const struct mgos_boot_slot *s = &cfg->slots[i];
    /* Can never return an active slot. */
    if (i == cfg->active_slot) continue;
    /* Must match map address, if specified. */
    if (app_map_addr != 0 && s->cfg.app_map_addr != app_map_addr) continue;
    /* Make sure there is a filesystem slot, if needed. */
    if (want_fs && s->cfg.fs_dev[0] == '\0') continue;
    /* If user doesn't want a particular slot, skip it. */
    if (i == excl1 || i == excl2) continue;
    /* Must be a valid writeable slot. */
    if (!(s->cfg.flags & MGOS_BOOT_SLOT_F_VALID)) continue;
    if (!(s->cfg.flags & MGOS_BOOT_SLOT_F_WRITEABLE)) continue;
    /* We have a candidate. see it it's better than what we have. */
    if (res < 0 || s->state.err_count < cfg->slots[res].state.err_count) {
      res = i;
    }
  }
  return res;
}

#ifdef MGOS_BOOT_BUILD
static bool mgos_boot_cfg_write_default(void) {
  bool res = false;
  mgos_boot_dbg_printf("Writing default config...\n");
  if (mgos_vfs_dev_erase(s_bcfg0_dev, 0, mgos_vfs_dev_get_size(s_bcfg0_dev)) !=
      0) {
    goto out;
  }
  if (s_bcfg1_dev != NULL &&
      mgos_vfs_dev_erase(s_bcfg1_dev, 0, mgos_vfs_dev_get_size(s_bcfg1_dev)) !=
          0) {
    goto out;
  }
  mgos_boot_cfg_set_default(s_bcfg);
  res = mgos_boot_cfg_write(s_bcfg, false /* dump*/);
out:
  return res;
}

bool mgos_boot_cfg_init(void) {
  bool res = false;
  s_bcfg = calloc(1, sizeof(*s_bcfg));
  s_bcfg0_dev = mgos_vfs_dev_open(MGOS_BOOT_CFG_DEV_0);
  s_bcfg1_dev = mgos_vfs_dev_open(MGOS_BOOT_CFG_DEV_1);
  if (s_bcfg0_dev == NULL && s_bcfg1_dev == NULL) {
    mgos_boot_dbg_printf("No config devs!\n");
    goto out;
  }

  if (mgos_boot_cfg_should_write_default()) {
    if (!mgos_boot_cfg_write_default()) goto out;
  }

  res = mgos_boot_cfg_find_latest();
  if (res) {
    mgos_boot_dbg_printf("Cfg @ %s:%lu\n", s_bcfg_dev->name,
                         (unsigned long) s_bcfg_off);
  } else {
    /* Read again after writing */
    res = mgos_boot_cfg_write_default() && mgos_boot_cfg_find_latest();
  }
out:
  if (!res) {
    free(s_bcfg);
    s_bcfg = NULL;
  }
  return res;
}
#elif defined(MGOS_BL_ENABLE)
bool mgos_boot_cfg_init(void) {
  bool res = false;
  s_bcfg0_dev = mgos_vfs_dev_open(MGOS_BOOT_CFG_DEV_0);
  s_bcfg1_dev = mgos_vfs_dev_open(MGOS_BOOT_CFG_DEV_1);
  if (s_bcfg0_dev == NULL && s_bcfg1_dev == NULL) {
    goto out;
  }

  struct mgos_boot_cfg_record *cfgr = &g_boot_state.cfgr;
  uint32_t crc = cs_crc32(0, cfgr, sizeof(*cfgr) - sizeof(uint32_t));
  if (crc != cfgr->crc32) goto out;
  if (strcmp(g_boot_state.cfg_dev, s_bcfg0_dev->name) == 0) {
    s_bcfg_dev = s_bcfg0_dev;
    s_bcfg_off = g_boot_state.cfg_off;
  } else if (strcmp(g_boot_state.cfg_dev, s_bcfg0_dev->name) == 0) {
    s_bcfg_dev = s_bcfg1_dev;
    s_bcfg_off = g_boot_state.cfg_off;
  } else {
    LOG(LL_ERROR,
        ("Boot loader uses unknown config dev %s!", g_boot_state.cfg_dev));
    goto out;
  }
  s_bcfg = &cfgr->cfg;
  LOG(LL_DEBUG, ("Cfg @ %s:%u", s_bcfg_dev->name, s_bcfg_off));
  mgos_boot_cfg_dump(s_bcfg);
  res = true;

out:
  return res;
}
#endif /* MGOS_BOOT_BUILD, MGOS_BL_ENABLE */

void mgos_boot_cfg_deinit(void) {
  mgos_vfs_dev_close(s_bcfg0_dev);
  mgos_vfs_dev_close(s_bcfg1_dev);
}

void mgos_boot_set_next_app_org(uintptr_t app_org) {
/* Only boot loader gets to set the magic, app only sets the value
 * because it must be understood by the loader. */
#ifdef MGOS_BOOT_BUILD
  g_boot_state.magic = MGOS_BOOT_CFG_MAGIC;
#endif
  g_boot_state.next_app_org = app_org;
}

#ifdef MGOS_BOOT_BUILD
uintptr_t mgos_boot_get_next_app_org(void) {
  if (g_boot_state.magic != MGOS_BOOT_CFG_MAGIC) return 0;
  return g_boot_state.next_app_org;
}
#endif

#else /* defined(MGOS_BL_ENABLE) || defined(MGOS_BOOT_BUILD) */
bool mgos_boot_cfg_init(void) {
  return false;
}

struct mgos_boot_cfg *mgos_boot_cfg_get(void) {
  return NULL;
}

bool mgos_boot_cfg_write(struct mgos_boot_cfg *cfg, bool dump) {
  (void) cfg;
  (void) dump;
  return false;
}

int8_t mgos_boot_cfg_find_slot(const struct mgos_boot_cfg *cfg,
                               uintptr_t app_map_addr, bool want_fs,
                               int8_t excl1, int8_t excl2) {
  (void) cfg;
  (void) app_map_addr;
  (void) want_fs;
  (void) excl1;
  (void) excl2;
  return -1;
}

#endif /* defined(MGOS_BL_ENABLE) || defined(MGOS_BOOT_BUILD) */

/* NB: Do not put init code here. This is invoked too late in app mode
 * and not at all in boot loader. */
bool mgos_bootloader_init(void) {
  return true;
}
