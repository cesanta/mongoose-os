#ifndef SJ_V7_EXT_INCLUDED
#define SJ_V7_EXT_INCLUDED

#include <v7.h>

/* Initialize objects and functions provided by v7_ext */
void sj_init_v7_ext(struct v7 *v7);

/* Initialize simple http client */
void sj_init_simple_http_client(struct v7 *v7);
void sj_http_error_callback(struct v7 *v7, v7_val_t cb, int err_no);
void sj_http_success_callback(struct v7 *v7, v7_val_t cb, const char *data,
                              size_t data_len);

/* Helper, calls arg function in context of v7 interpretator */
void sj_call_function(struct v7 *v7, void *arg);

#endif
