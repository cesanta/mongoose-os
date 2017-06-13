/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_UPDATER_UTIL_H_
#define CS_FW_SRC_MGOS_UPDATER_UTIL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Function to merge filesystems. */
bool mgos_upd_merge_fs(const char *old_fs_path, const char *new_fs_path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_UTIL_H_ */
