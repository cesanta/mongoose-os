#ifndef CS_FW_SRC_SJ_APP_H_
#define CS_FW_SRC_SJ_APP_H_

enum mg_app_init_result {
  MG_APP_INIT_SUCCESS = 0,
  MG_APP_INIT_ERROR = -2,
};

/* User app init functions, C and JS respectively.
 * A weak stub is provided in sj_app_init.c, which can be overridden. */
enum mg_app_init_result sj_app_init();
#ifdef SJ_ENABLE_JS
struct v7;
enum mg_app_init_result sj_app_init_js(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_SJ_APP_H_ */
