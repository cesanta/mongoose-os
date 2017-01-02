/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include <string.h>

#include "fw/src/mgos_dlsym.h"

extern struct mgos_ffi_export ffi_exports[];
extern int ffi_exports_cnt;

void *mgos_dlsym(void *handle, const char *name) {
  (void) handle;
  int i;
  for (i = 0; i < ffi_exports_cnt; i++) {
    if (strcmp(name, ffi_exports[i].name) == 0) return ffi_exports[i].addr;
  }
  return NULL;
}
