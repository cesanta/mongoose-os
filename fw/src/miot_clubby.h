/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_CLUBBY_H_
#define CS_FW_SRC_MIOT_CLUBBY_H_

#include "fw/src/miot_features.h"

#if MG_ENABLE_CLUBBY

#include "common/clubby/clubby.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_sys_config.h"

enum miot_init_result miot_clubby_init(void);
struct clubby *miot_clubby_get_global(void);
struct clubby_cfg *miot_clubby_cfg_from_sys(const struct sys_config *scfg);
struct clubby_channel_ws_out_cfg *miot_clubby_channel_ws_out_cfg_from_sys(
    const struct sys_config *scfg);

#endif /* MG_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MIOT_CLUBBY_H_ */
