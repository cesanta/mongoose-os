/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

#pragma once

#include "mgos_ota.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Start OTA update by pulling the firmware from the given URL. */
void mgos_ota_http_start(const char *url, const struct mgos_ota_opts *opts);

#ifdef __cplusplus
}
#endif
