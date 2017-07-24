/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos.h"
#include "test_bin_lib.h"

enum mgos_app_init_result mgos_app_init(void) {
  LOG(LL_INFO, ("TEST LIB VALUE: %d", test_bin_lib_value()));

  return MGOS_APP_INIT_SUCCESS;
}
