
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef SJ_DEBUG_H_INCLUDED
#define SJ_DEBUG_H_INCLUDED

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

#endif /* SJ_DEBUG_H_INCLUDED */
