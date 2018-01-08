/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_UPDATER_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_UPDATER_H_

#if MGOS_ENABLE_UPDATER

#ifdef __cplusplus
extern "C" {
#endif

bool cc3200_upd_init(void);
const char *cc3200_upd_get_fs_container_prefix(void);

#ifdef __cplusplus
}
#endif

#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_UPDATER_H_ */
