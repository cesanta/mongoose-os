/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_H_
#define CS_FW_SRC_MG_CLUBBY_H_

#ifdef SJ_ENABLE_CLUBBY

#include "common/clubby/clubby.h"
#include "fw/src/sj_init.h"
#include "fw/src/sj_sys_config.h"

enum sj_init_result mg_clubby_init(void);
struct clubby *mg_clubby_get_global(void);
struct clubby_cfg *mg_clubby_cfg_from_sys(
    const struct sys_config_clubby *sccfg);
struct clubby_channel_ws_out_cfg *mg_clubby_channel_ws_out_cfg_from_sys(
    const struct sys_config_clubby *sccfg);

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MG_CLUBBY_H_ */
