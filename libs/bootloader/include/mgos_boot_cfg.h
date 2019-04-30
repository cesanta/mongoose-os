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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mgos_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MGOS_BOOT_CFG_NUM_SLOTS
#define MGOS_BOOT_CFG_NUM_SLOTS 5
#endif

/*
 * Things to consider when changing the size of the record.
 *  - Backward compatibility.
 *  - Must be multiple of 16 to be AES-compatible (16 byte block size).
 */
#ifndef MGOS_BOOT_CFG_REC_SIZE
#define MGOS_BOOT_CFG_REC_SIZE 256
#endif

#ifndef MGOS_BOOT_CFG_MAX_RECS_PER_DEV
#define MGOS_BOOT_CFG_MAX_RECS_PER_DEV 32
#endif

#define MGOS_BOOT_CFG_MAGIC 0x31534f4d /* "MOS1" LE */

struct mgos_boot_slot_cfg {
  uint32_t flags;
  char app_dev[8];
  uintptr_t app_map_addr; /* For slots that are directly memory mapped,
                             the address of the mapping. */
  char fs_dev[8];
} __attribute__((packed));

#define MGOS_BOOT_SLOT_F_VALID (1 << 0)
#define MGOS_BOOT_SLOT_F_WRITEABLE (1 << 1)

struct mgos_boot_slot_state {
  uint32_t app_len;   /* Length of the app data in the slot */
  uintptr_t app_org;  /* Origin address of the firmware in this slot.
                         If state.app_org == config.map_addr, then the slot is
                         directly bootable. */
  uint32_t app_crc32; /* CRC32 of the app data in the slot */
  uint32_t app_flags; /* Flags used by app in this slot. */
  uint8_t err_count;  /* Indication of "badness". */
} __attribute__((packed));

#define MGOS_BOOT_APP_F_FS_CREATED (1 << 0)

struct mgos_boot_slot {
  struct mgos_boot_slot_cfg cfg;
  struct mgos_boot_slot_state state;
} __attribute__((packed));

struct mgos_boot_cfg {
  uint32_t magic;
  uint32_t seq; /* Sequencer. Increasing value = newer. */
  uint32_t version;
  uint8_t num_slots;
  int8_t active_slot;
  int8_t revert_slot;
  uint32_t flags;
  struct mgos_boot_slot slots[MGOS_BOOT_CFG_NUM_SLOTS];
} __attribute__((packed));

#define MGOS_BOOT_F_COMMITTED (1 << 0)
#define MGOS_BOOT_F_FIRST_BOOT_A (1 << 1)
#define MGOS_BOOT_F_FIRST_BOOT_B (1 << 2)
#define MGOS_BOOT_F_MERGE_FS (1 << 3)

#define MGOS_BOOT_CFG_REC_PADDING_SIZE \
  (MGOS_BOOT_CFG_REC_SIZE - sizeof(struct mgos_boot_cfg) - sizeof(uint32_t))

struct mgos_boot_cfg_record {
  struct mgos_boot_cfg cfg;
  uint8_t padding[MGOS_BOOT_CFG_REC_PADDING_SIZE];
  uint32_t crc32; /* CRC32 of the fields above. */
} __attribute__((packed));

#define MGOS_BOOT_STATE_SIZE 1024
#define MGOS_BOOT_STATE_PADDING_SIZE                                \
  (MGOS_BOOT_STATE_SIZE - sizeof(struct mgos_boot_cfg_record) - 8 - \
   3 * sizeof(uint32_t))

/* This state is located at a static location in memory and is populated
 * by the boot loader. */
struct mgos_boot_state {
  /* If next_app_org is non-zero and magic is valid, boot loader will skip
   * everything and boot app at this address (using mgos_boot_app).
   * This is a one-shot action: before jumping the address will be zeroed,
   * so next boot will go into boot loader. */
  uint32_t magic;
  uintptr_t next_app_org;
  /* Location of the most recent config record: device name and offset. */
  char cfg_dev[8];
  uint32_t cfg_off;
  /* The most recent config record used by the loader. App will use this. */
  struct mgos_boot_cfg_record cfgr;
  uint8_t padding[MGOS_BOOT_STATE_PADDING_SIZE];
} __attribute__((packed));

#define MGOS_BOOT_CFG_DEV_0 "bcfg0"
#define MGOS_BOOT_CFG_DEV_1 "bcfg1"

void mgos_boot_set_next_app_org(uintptr_t app_org);

#ifdef MGOS_BOOT_BUILD
uintptr_t mgos_boot_get_next_app_org(void);
#endif

bool mgos_boot_cfg_init(void);

struct mgos_boot_cfg *mgos_boot_cfg_get(void);

void mgos_boot_cfg_set_default_slots(struct mgos_boot_cfg *cfg);

void mgos_boot_cfg_dump(const struct mgos_boot_cfg *cfg);

bool mgos_boot_cfg_write(struct mgos_boot_cfg *cfg, bool dump);

int8_t mgos_boot_cfg_find_slot(const struct mgos_boot_cfg *cfg,
                               uintptr_t app_map_addr, bool want_fs,
                               int8_t excl1, int8_t excl2);

void mgos_boot_cfg_deinit(void);

bool mgos_boot_cfg_should_write_default(void);

#ifdef __cplusplus
}
#endif

CS_CTASSERT(sizeof(struct mgos_boot_cfg_record) == MGOS_BOOT_CFG_REC_SIZE,
            do_not_change_size_of_boot_config_record);
CS_CTASSERT(sizeof(struct mgos_boot_state) == MGOS_BOOT_STATE_SIZE,
            do_not_change_size_of_boot_state);
