#include <stdlib.h>
#include <v7.h>
#include <fossa.h>
#include <sj_hal.h>
#include <sj_v7_ext.h>
static struct ns_mgr mgr;

struct user_data {
  struct v7 *v7;
  v7_val_t cb;
};

static void fossa_ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  struct user_data *ud = (struct user_data *) nc->user_data;
  int connect_status;

  switch (ev) {
    case NS_CONNECT:
      connect_status = *(int *) ev_data;
      if (connect_status != 0) {
        sj_http_error_callback(ud->v7, ud->cb, connect_status);
        free(ud);
        nc->user_data = NULL;
      }
      break;
    case NS_HTTP_REPLY:
      sj_http_success_callback(ud->v7, ud->cb, hm->body.p, hm->body.len);
      free(ud);
      nc->user_data = NULL;
      nc->flags |= NSF_SEND_AND_CLOSE;
      break;
    case NS_CLOSE:
      if (ud != NULL) {
        /*
         * Something goes wrong: ud would be NULL if
         * connection closed after receiving reply
         */
        sj_http_error_callback(ud->v7, ud->cb, -3);
        free(ud);
      }
      break;
  }
}

void init_fossa() {
  ns_mgr_init(&mgr, NULL);
}

void destroy_fossa() {
  ns_mgr_free(&mgr);
}

int poll_fossa() {
  if (ns_next(&mgr, NULL) != NULL) {
    ns_mgr_poll(&mgr, 1000);
    return 1;
  } else {
    return 0;
  }
}

int sj_http_call(struct v7 *v7, const char *url, const char *body,
                 size_t body_len, const char *method, v7_val_t cb) {
  struct ns_connection *nc;
  struct user_data *ud = NULL;

  nc = ns_connect_http(&mgr, fossa_ev_handler, url, 0, body);

  if (nc == NULL) {
    sj_http_error_callback(v7, cb, -2);
    return 1;
  }

  ud = calloc(1, sizeof(*ud));
  ud->v7 = v7;
  ud->cb = cb;
  nc->user_data = ud;

  return 0;
}
