/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_UPDATER_UTIL_H_
#define CS_FW_SRC_MIOT_UPDATER_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Function to merge filesystems. */
struct spiffs_t;
int miot_upd_merge_spiffs(struct spiffs_t *old_fs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_UPDATER_UTIL_H_ */
