/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_SERVICE_VARS_H_
#define CS_FW_SRC_MG_SERVICE_VARS_H_

#if defined(MG_ENABLE_CLUBBY) && defined(MG_ENABLE_CONFIG_SERVICE)

#include "fw/src/mg_init.h"

/*
 * Initialises clubby handlers for /v1/Vars commands
 */
enum mg_init_result mg_service_vars_init(void);

#endif /* MG_ENABLE_CLUBBY && MG_ENABLE_CONFIG_SERVICE */
#endif /* CS_FW_SRC_MG_SERVICE_VARS_H_ */
