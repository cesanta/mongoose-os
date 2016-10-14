/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef WINCE

const char *strerror(int err) {
  /*
   * TODO(alashkin): there is no strerror on WinCE;
   * look for similar wce_xxxx function
   */
  static char buf[10];
  snprintf(buf, sizeof(buf), "%d", err);
  return buf;
}

#endif