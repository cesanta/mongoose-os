/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_MG_UPDATER_CLUBBY_H_
#define CS_FW_SRC_MG_UPDATER_CLUBBY_H_

#include <inttypes.h>
#include "common/mg_str.h"
#include "fw/src/mg_features.h"

#if MG_ENABLE_UPDATER_CLUBBY && MG_ENABLE_CLUBBY
void mg_updater_clubby_init(void);
void mg_updater_clubby_finish(int error_code, int64_t id,
                              const struct mg_str src);
#endif

#endif /* CS_FW_SRC_MG_UPDATER_CLUBBY_H_ */
