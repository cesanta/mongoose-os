/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MONGOOSE_INTERNAL_H_
#define CS_FW_SRC_MGOS_MONGOOSE_INTERNAL_H_

#include <stdbool.h>

#include "mgos_init.h"

#include "mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Initialize global event manager
 */
enum mgos_init_result mongoose_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MONGOOSE_INTERNAL_H_ */
