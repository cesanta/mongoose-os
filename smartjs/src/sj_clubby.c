#include "sj_clubby.h"
#include "clubby_proto.h"
#include "sj_mongoose.h"
#include "device_config.h"

#ifndef DISABLE_C_CLUBBY

#define MAX_COMMAND_NAME_LENGTH 15

struct clubby_cb_info {
  char id[MAX_COMMAND_NAME_LENGTH];
  int8_t id_len;

  clubby_callback cb;
  void *user_data;

  struct clubby_cb_info *next;
};

static struct clubby_cb_info *s_resp_cbs;

static int clubby_register_callback(char *id, int8_t id_len, clubby_callback cb,
                                    void *user_data) {
  struct clubby_cb_info *new_cb_info = calloc(1, sizeof(struct clubby_cb_info));

  if (new_cb_info == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return 0;
  }

  if (id_len > MAX_COMMAND_NAME_LENGTH) {
    LOG(LL_ERROR, ("ID too long (%d)", id_len));
    return 0;
  }

  new_cb_info->next = s_resp_cbs;
  memcpy(new_cb_info->id, id, id_len);
  new_cb_info->id_len = id_len;
  new_cb_info->cb = cb;
  new_cb_info->user_data = user_data;

  s_resp_cbs = new_cb_info;

  return 1;
}

static void clubby_remove_callback(char *id, int8_t id_len) {
  struct clubby_cb_info *current = s_resp_cbs;
  struct clubby_cb_info *prev = NULL;

  while (current != NULL) {
    if (memcmp(current->id, id, id_len) == 0) {
      if (prev == NULL) {
        s_resp_cbs = current->next;
      } else {
        prev->next = current->next;
      }

      free(current);
      break;
    }

    prev = current;
    current = current->next;
  }
}

/*
 * The only reason to have two functions (remove_resp_callback(
 * and get_resp_callback is that _now_ I'm not sure if we always have
 * to delete cb info immediately after invocation
 */
static struct clubby_cb_info *clubby_find_callback(const char *id,
                                                   int8_t id_len) {
  struct clubby_cb_info *current = s_resp_cbs;

  while (current != NULL) {
    if (memcmp(current->id, id, id_len) == 0) {
      break;
    }

    current = current->next;
  }

  return current;
}

/* Using separated callback for /v1/Hello in demo and debug purposes */
static void clubby_hello_resp_callback(struct clubby_event *evt) {
  LOG(LL_DEBUG,
      ("Got response for /v1/Hello, status=%d", evt->response.status));
}

static void clubby_hello_req_callback(struct clubby_event *evt) {
  LOG(LL_DEBUG, ("Incoming /v1/Hello received, id=%d", evt->request.id));
  char src[100] = {0};
  if (evt->request.src->len > sizeof(src)) {
    LOG(LL_ERROR, ("src too long, len=%d", evt->request.src->len));
    return;
  }
  memcpy(src, evt->request.src->ptr, evt->request.src->len);

  char status_msg[100];
  snprintf(status_msg, sizeof(status_msg) - 1, "Hello, this is %s",
           get_cfg()->clubby.device_id);

  clubby_proto_send_resp(src, evt->request.id, 0, status_msg);
}

static void clubby_send_hello() {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_val_t cmdv = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmdv);
  ub_add_prop(ctx, cmdv, "cmd", ub_create_string("/v1/Hello"));
  int32_t id = clubby_proto_get_new_id();
  ub_add_prop(ctx, cmdv, "id", ub_create_number(id));
  clubby_register_callback((char *) &id, sizeof(id), clubby_hello_resp_callback,
                           NULL);
  clubby_proto_send_cmd(ctx, get_cfg()->clubby.backend, cmds);
}

static void clubby_cb(struct clubby_event *evt) {
  switch (evt->ev) {
    case CLUBBY_NET_CONNECT: {
      /* TODO(alashkin): handle connection error here */
      LOG(LL_DEBUG, ("CLUBBY_NET_CONNECT"));
      break;
    }
    case CLUBBY_CONNECT: {
      LOG(LL_DEBUG, ("CLUBBY_CONNECT"));
      clubby_send_hello();
      break;
    }

    case CLUBBY_DISCONNECT: {
      /* TODO(alashkin): handle disconnection here */
      LOG(LL_DEBUG, ("CLUBBY_DISCONNECT"));
      break;
    }

    case CLUBBY_RESPONSE: {
      LOG(LL_DEBUG,
          ("CLUBBY_RESPONSE: id=%d status=%d "
           "status_msg=%.*s resp=%.*s",
           (int32_t) evt->response.id, evt->response.status,
           evt->response.status_msg ? evt->response.status_msg->len : 0,
           evt->response.status_msg ? evt->response.status_msg->ptr : "",
           evt->response.resp ? evt->response.resp->len : 0,
           evt->response.resp ? evt->response.resp->ptr : ""));

      struct clubby_cb_info *cb_info = clubby_find_callback(
          (char *) &evt->response.id, sizeof(evt->response.id));

      if (cb_info != NULL) {
        evt->user_data = cb_info->user_data;
        cb_info->cb(evt);
        clubby_remove_callback((char *) &evt->response.id,
                               sizeof(evt->response.id));
      }

      break;
    }

    case CLUBBY_REQUEST: {
      LOG(LL_DEBUG, ("CLUBBY_REQUEST: id=%d cmd=%.*s", evt->request.id,
                     evt->request.cmd->len, evt->request.cmd->ptr));

      struct clubby_cb_info *cb_info =
          clubby_find_callback(evt->request.cmd->ptr, evt->request.cmd->len);

      if (cb_info != NULL) {
        evt->user_data = cb_info->user_data;
        cb_info->cb(evt);
      } else {
        LOG(LL_DEBUG, ("Unregistered command"));
      }

      break;
    }

    case CLUBBY_FRAME:
      /* Don't want to work on this abstraction level */
      break;
  }
}

void sj_init_clubby(struct v7 *v7) {
  /* TODO(alashkin) : add JS bindings */
  (void) v7;

  clubby_proto_init(clubby_cb);
  clubby_register_callback("/v1/Hello", 9, clubby_hello_req_callback, NULL);

  clubby_proto_connect(&sj_mgr);
}

#endif /* DISABLE_C_CLUBBY */
