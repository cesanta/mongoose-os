#include "sj_clubby.h"
#include "clubby_proto.h"
#include "sj_mongoose.h"
#include "device_config.h"

#ifndef DISABLE_C_CLUBBY

static void clubby_send_hello() {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_val_t cmdv = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmdv);
  ub_add_prop(ctx, cmdv, "cmd", ub_create_string("/v1/Hello"));
  ub_add_prop(ctx, cmdv, "id", ub_create_number(clubby_proto_get_new_id()));
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

    case CLUBBY_RESPONSE: {
      /* TODO(alashkin): handle response here */
      LOG(LL_DEBUG,
          ("CLUBBY_RESPONSE: id=%d status=%d "
           "status_msg=%.*s resp=%.*s",
           evt->response.id, evt->response.status,
           evt->response.status_msg ? evt->response.status_msg->len : 0,
           evt->response.status_msg ? evt->response.status_msg->ptr : "",
           evt->response.resp ? evt->response.resp->len : 0,
           evt->response.resp ? evt->response.resp->ptr : ""));
      break;
    }

    case CLUBBY_FRAME:
      /* Don't want to work on this abstraction level */
      break;

    default: {
      LOG(LL_DEBUG, ("Unhandled clubby event: %d", (int) evt->ev));
      break;
    }
  }
}

void sj_init_clubby(struct v7 *v7) {
  /* TODO(alashkin) : add JS bindings */
  (void) v7;

  clubby_proto_init(clubby_cb);
  clubby_proto_connect(&sj_mgr);
}

#endif /* DISABLE_C_CLUBBY */
