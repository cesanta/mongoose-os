/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Registers a POST file upload HTTP handler that accepts firmware ZIP bundles.
 */

#ifndef CS_FW_SRC_SJ_UPDATER_POST_H_
#define CS_FW_SRC_SJ_UPDATER_POST_H_

#ifdef SJ_ENABLE_UPDATER_POST
void sj_updater_post_init(void);
#endif

#endif /* CS_FW_SRC_SJ_UPDATER_POST_H_ */
