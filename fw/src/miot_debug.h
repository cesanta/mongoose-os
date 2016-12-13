
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_DEBUG_H_
#define CS_FW_SRC_MIOT_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Debug modes.
 *
 * Exact numbers are part of the existing API, so, they are specified here
 * explicitly.
 */
enum debug_mode {
  DEBUG_MODE_OFF = 0,
  DEBUG_MODE_STDOUT = 1,
  DEBUG_MODE_STDERR = 2,
};

int miot_debug_redirect(enum debug_mode mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_DEBUG_H_ */
