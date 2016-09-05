/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_SERVICE_CONFIG_H_
#define CS_FW_SRC_SJ_SERVICE_CONFIG_H_

#if defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_CONFIG_SERVICE)

#include "fw/src/sj_init.h"

/*
 * Initialises clubby handlers for /v1/Config commands
 */
enum sj_init_result sj_service_config_init(void);

#endif /* SJ_ENABLE_CLUBBY && SJ_ENABLE_CONFIG_SERVICE */
#endif /* CS_FW_SRC_SJ_SERVICE_CONFIG_H_ */
