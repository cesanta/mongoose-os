/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_HTTP_H_
#define CS_FW_SRC_SJ_HTTP_H_

#if defined(SJ_ENABLE_HTTP_CLIENT_API) || defined(SJ_ENABLE_HTTP_SERVER_API)
struct v7;
void sj_http_api_setup(struct v7 *v7);
void sj_http_js_init(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_SJ_HTTP_H_ */
