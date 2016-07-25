/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_SJ_UPDATER_CLUBBY_JS_H_
#define CS_FW_SRC_SJ_UPDATER_CLUBBY_JS_H_

#if defined(SJ_ENABLE_JS) && defined(SJ_ENABLE_CLUBBY)
struct v7;
void sj_updater_clubby_js_init(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_SJ_UPDATER_CLUBBY_JS_H_ */
