/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_ONEWIRE_H_
#define CS_FW_SRC_MGOS_ONEWIRE_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_ONEWIRE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_onewire_search_state {
  int search_mode;
  int last_device;
  int last_discrepancy;
  int last_family_discrepancy;
  unsigned char rom[8];
  unsigned char crc8;
};

struct mgos_onewire {
  int pin;
  unsigned char *res_rom;
  struct mgos_onewire_search_state sst;
};

unsigned char mgos_onewire_crc8(const unsigned char *rom, unsigned int len);
int mgos_onewire_reset(struct mgos_onewire *ow);
void mgos_onewire_target_setup(struct mgos_onewire *ow,
                               const unsigned char family_code);
int mgos_onewire_next(struct mgos_onewire *ow, unsigned char *rom, int mode);
void mgos_onewire_select(struct mgos_onewire *ow, const unsigned char *rom);
void mgos_onewire_skip(struct mgos_onewire *ow);
void mgos_onewire_search_clean(struct mgos_onewire *ow);
int mgos_onewire_read_bit(struct mgos_onewire *ow);
unsigned char mgos_onewire_read(struct mgos_onewire *ow);
void mgos_onewire_read_bytes(struct mgos_onewire *ow, unsigned char *buf,
                             unsigned int len);
void mgos_onewire_write_bit(struct mgos_onewire *ow, int bit);
void mgos_onewire_write(struct mgos_onewire *ow, const unsigned char data);
void mgos_onewire_write_bytes(struct mgos_onewire *ow, const unsigned char *buf,
                              unsigned int len);
struct mgos_onewire *mgos_onewire_init(int pin);
void mgos_onewire_close(struct mgos_onewire *ow);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_ONEWIRE */

#endif /* CS_FW_SRC_MGOS_ONEWIRE_H_ */
