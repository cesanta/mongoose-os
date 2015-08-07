#include <stdlib.h>
#include <v7.h>
#include <fossa.h>
#include <sj_hal.h>
#include <sj_v7_ext.h>
static struct ns_mgr mgr;

struct user_data {
  struct v7 *v7;
  char *body;
  char *path;
  char method[10];
  v7_val_t cb;
};

static void free_user_data(struct user_data *ud) {
  if (ud != NULL) {
    free(ud->path);
    free(ud->body);
    free(ud);
  }
}

static void fossa_ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  struct user_data *ud = (struct user_data *) nc->user_data;
  int connect_status;

  switch (ev) {
    case NS_CONNECT:
      connect_status = *(int *) ev_data;
      if (connect_status == 0) {
        if (strcmp(ud->method, "GET") == 0) {
          const char *const reqfmt = "GET %s HTTP/1.0\r\n\r\n";
          ns_printf(nc, reqfmt, ud->path);
        } else {
          const char *const reqfmt =
              "POST %s HTTP/1.0\r\nContent-Length: %d\r\n\r\n%s";
          size_t body_len = strlen(ud->body);
          ns_printf(nc, reqfmt, ud->path, (int) body_len, ud->body);
        }
      } else {
        sj_http_error_callback(ud->v7, ud->cb, connect_status);
        free_user_data(ud);
      }
      break;
    case NS_HTTP_REPLY:
      sj_http_success_callback(ud->v7, ud->cb, hm->body.p, hm->body.len);
      free_user_data(ud);
      nc->flags |= NSF_SEND_AND_CLOSE;
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
  if (mgr.active_connections != NULL) {
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

  ud = calloc(1, sizeof(*ud));
  ud->v7 = v7;
  ud->cb = cb;

  if (memcmp(url, "http://", 7) == 0) {
    /* + some space for port number */
    size_t path_len = strlen(url) + 10;
    ud->path = calloc(1, path_len);
    snprintf(ud->path, path_len, "tcp://%s:80", url + 7);
  } else if (memcmp(url, "https://", 8) == 0) {
    /* TODO(alashkin): add HTTPs setup here */
    fprintf(stderr, "NOT IMPLEMENTED\n");
    goto err;
  } else {
    fprintf(stderr, "Address must be either http or https\n");
    goto err;
  }

  nc = ns_connect(&mgr, ud->path, fossa_ev_handler);

  if (nc == NULL) {
    fprintf(stderr, "Invalid address: %s\n", url);
    goto err;
  }

  ns_set_protocol_http_websocket(nc);

  if (body != NULL) {
    ud->body = calloc(1, body_len + 1);
    memcpy(ud->body, body, body_len);
  }

  strcpy(ud->method, method);
  nc->user_data = ud;

  return 1;

err:
  free_user_data(ud);

  return 0;
}
