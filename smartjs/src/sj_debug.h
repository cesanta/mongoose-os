
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_DEBUG_H_
#define CS_SMARTJS_SRC_SJ_DEBUG_H_

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

int sj_debug_redirect(enum debug_mode mode);

#endif /* CS_SMARTJS_SRC_SJ_DEBUG_H_ */
