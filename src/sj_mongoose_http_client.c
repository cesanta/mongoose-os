/*
 * Implements mongoose platform specific sj_http_call.
 * This is common to all platforms that use mongoose as networking API
 */
#include <stdlib.h>
#include <v7.h>
#include <mongoose.h>
#include <sj_hal.h>
#include <sj_v7_ext.h>
#include <sj_mongoose.h>

struct user_data {
  struct v7 *v7;
  v7_val_t cb;
};

static void mongoose_ev_handler(struct mg_connection *nc, int ev,
                                void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  struct user_data *ud = (struct user_data *) nc->user_data;
  int connect_status;

  switch (ev) {
    case MG_EV_CONNECT:
      connect_status = *(int *) ev_data;
      if (connect_status != 0) {
        sj_http_error_callback(ud->v7, ud->cb, connect_status);
        v7_disown(ud->v7, &ud->cb);
        free(ud);
        nc->user_data = NULL;
      }
      break;
    case MG_EV_HTTP_REPLY:
      sj_http_success_callback(ud->v7, ud->cb, hm->body.p, hm->body.len);
      v7_disown(ud->v7, &ud->cb);
      free(ud);
      nc->user_data = NULL;
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    case MG_EV_CLOSE:
      if (ud != NULL) {
        /*
         * Something goes wrong: ud would be NULL if
         * connection closed after receiving reply
         */
        sj_http_error_callback(ud->v7, ud->cb, -3);
        v7_disown(ud->v7, &ud->cb);
        free(ud);
      }
      break;
  }
}

int sj_http_call(struct v7 *v7, const char *url, const char *body,
                 size_t body_len, const char *method, v7_val_t cb) {
  struct mg_connection *nc;
  struct user_data *ud = NULL;

  nc = mg_connect_http(&sj_mgr, mongoose_ev_handler, url, 0, body);

  if (nc == NULL) {
    sj_http_error_callback(v7, cb, -2);
    return 0;
  }

  ud = calloc(1, sizeof(*ud));
  ud->v7 = v7;
  ud->cb = cb;
  v7_own(v7, &ud->cb);
  nc->user_data = ud;

  return 1;
}
