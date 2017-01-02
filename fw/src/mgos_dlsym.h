/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_FFI_H_
#define CS_FW_SRC_MGOS_FFI_H_

#include "fw/src/mgos_features.h"

struct mgos_ffi_export {
  const char *name;
  void *addr;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void *mgos_dlsym(void *handle, const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_FFI_H_ */
