#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/queue.h"
#include "common/str_util.h"
#include "frozen.h"

#include "mgos_app.h"
#include "mgos_crontab.h"
#include "mgos_rpc.h"

#define ITEM_FMT "{id: %d, at: %T, enable: %B, action: %T, payload: %T}"

/*
 * Returns newly allocated unescaped string
 */
static struct mg_str create_unescaped(struct mg_str s) {
  int len = json_unescape(s.p, s.len, NULL, 0);
  char *data = (char *) calloc(1, len);
  json_unescape(s.p, s.len, data, len);
  return mg_mk_str_n(data, len);
}

/*
 * NOTE: `at` and `action` are reallocated (because they need unescaping),
 * so the caller should free them.
 */
static char *parse_job_json(struct mg_str json, struct mg_str *at, bool *enable,
                            struct mg_str *action, struct mg_str *payload) {
  char *err = NULL;

  *enable = true;
  struct json_token t_at = JSON_INVALID_TOKEN;
  struct json_token t_action = JSON_INVALID_TOKEN;
  struct json_token t_payload = JSON_INVALID_TOKEN;
  json_scanf(json.p, json.len, "{at: %T, enable: %B, action: %T, payload: %T}",
             &t_at, enable, &t_action, &t_payload);

  if (t_at.ptr == NULL || t_action.ptr == NULL) {
    mg_asprintf(&err, 0, "'at' and 'action' are required");
    goto clean;
  }

  *at = create_unescaped(mg_mk_str_n(t_at.ptr, t_at.len));
  *action = create_unescaped(mg_mk_str_n(t_action.ptr, t_action.len));
  *payload = mg_mk_str_n(t_payload.ptr, t_payload.len);

clean:
  return err;
}

static mgos_crontab_job_id_t parse_id_json(struct mg_str json) {
  mgos_crontab_job_id_t id = MGOS_CRONTAB_INVALID_JOB_ID;
  json_scanf(json.p, json.len, "{id: %d}", &id);
  return id;
}

static int marshal_job_json(struct json_out *out, mgos_crontab_job_id_t id,
                            struct mg_str at, bool enable, struct mg_str action,
                            struct mg_str payload) {
  int ret = 0;

  ret += json_printf(out, "{");
  ret += json_printf(out, "id: %d, at: %.*Q, enable: %B, action: %.*Q", id,
                     at.len, at.p, enable, action.len, action.p);
  if (payload.p != NULL && payload.len > 0) {
    ret += json_printf(out, ", payload: %.*s", payload.len, payload.p);
  }
  ret += json_printf(out, "}");

  return ret;
}

static void add_or_edit_job(bool edit, struct mg_rpc_request_info *ri,
                            void *cb_arg, struct mg_rpc_frame_info *fi,
                            struct mg_str args) {
  char *err = NULL;

  struct mg_str at = MG_NULL_STR;
  bool enable = false;
  struct mg_str action = MG_NULL_STR;
  struct mg_str payload = MG_NULL_STR;
  mgos_crontab_job_id_t id = MGOS_CRONTAB_INVALID_JOB_ID;

  /* Get crontab job params from JSON args */
  err = parse_job_json(args, &at, &enable, &action, &payload);
  if (err != NULL) {
    mg_rpc_send_errorf(ri, 400, "%s", err);
    ri = NULL;
    goto clean;
  }

  /* Add or edit the job */
  if (!edit) {
    mgos_crontab_job_add(at, enable, action, payload, &id, &err);
  } else {
    id = parse_id_json(args);
    mgos_crontab_job_edit(id, at, enable, action, payload, &err);
  }
  if (err != NULL) {
    mg_rpc_send_errorf(ri, 400, "%s", err);
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, "{id: %d}", id);
  ri = NULL;

clean:
  free(err);

  /*
   * `at` and `action` are reallocated by `parse_job_json`, so we need to
   * free them
   */
  free((char *) at.p);
  free((char *) action.p);

  (void) cb_arg;
  (void) fi;
}

static void rpc_handler_cron_add_item(struct mg_rpc_request_info *ri,
                                      void *cb_arg,
                                      struct mg_rpc_frame_info *fi,
                                      struct mg_str args) {
  return add_or_edit_job(false, ri, cb_arg, fi, args);
}

static void rpc_handler_cron_edit_item(struct mg_rpc_request_info *ri,
                                       void *cb_arg,
                                       struct mg_rpc_frame_info *fi,
                                       struct mg_str args) {
  return add_or_edit_job(true, ri, cb_arg, fi, args);
}

static void rpc_handler_cron_remove_item(struct mg_rpc_request_info *ri,
                                         void *cb_arg,
                                         struct mg_rpc_frame_info *fi,
                                         struct mg_str args) {
  char *err = NULL;

  mgos_crontab_job_id_t id = parse_id_json(args);
  mgos_crontab_job_remove(id, &err);
  if (err != NULL) {
    mg_rpc_send_errorf(ri, 400, "%s", err);
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, "{id: %d}", id);
  ri = NULL;

clean:
  free(err);

  (void) cb_arg;
  (void) fi;
}

static void rpc_handler_cron_get_item(struct mg_rpc_request_info *ri,
                                      void *cb_arg,
                                      struct mg_rpc_frame_info *fi,
                                      struct mg_str args) {
  char *err = NULL;

  struct mg_str at = MG_NULL_STR;
  bool enable = false;
  struct mg_str action = MG_NULL_STR;
  struct mg_str payload = MG_NULL_STR;

  struct mbuf databuf;
  mbuf_init(&databuf, 0);

  mgos_crontab_job_id_t id = parse_id_json(args);
  mgos_crontab_job_get(id, &at, &enable, &action, &payload, &err);
  if (err != NULL) {
    mg_rpc_send_errorf(ri, 400, "%s", err);
    ri = NULL;
    goto clean;
  }

  mbuf_init(&databuf, 50);
  {
    struct json_out out = JSON_OUT_MBUF(&databuf);

    marshal_job_json(&out, id, at, enable, action, payload);
  }

  mg_rpc_send_responsef(ri, "%.*s", databuf.len, databuf.buf);
  ri = NULL;

clean:
  free(err);

  /*
   * All string data is reallocated by mgos_crontab_job_get, so we need
   * to free it
   */
  free((char *) at.p);
  free((char *) action.p);
  free((char *) payload.p);
  mbuf_free(&databuf);

  (void) cb_arg;
  (void) fi;
}

struct list_ctx {
  struct json_out out;
  int idx;
};

static void list_cb(mgos_crontab_job_id_t id, struct mg_str at, bool enable,
                    struct mg_str action, struct mg_str payload,
                    void *userdata) {
  struct list_ctx *ctx = (struct list_ctx *) userdata;

  if (ctx->idx > 0) {
    json_printf(&ctx->out, ",");
  }

  marshal_job_json(&ctx->out, id, at, enable, action, payload);

  ctx->idx++;
}

static void rpc_handler_cron_list_items(struct mg_rpc_request_info *ri,
                                        void *cb_arg,
                                        struct mg_rpc_frame_info *fi,
                                        struct mg_str args) {
  char *err = NULL;

  struct mbuf databuf;
  mbuf_init(&databuf, 50);
  struct list_ctx ctx = {
      .out = JSON_OUT_MBUF(&databuf), .idx = 0,
  };

  json_printf(&ctx.out, "[");
  mgos_crontab_iterate(list_cb, &ctx, &err);
  if (err != NULL) {
    mg_rpc_send_errorf(ri, 500, "%s", err);
    ri = NULL;
    goto clean;
  }
  json_printf(&ctx.out, "]");

  mg_rpc_send_responsef(ri, "%.*s", databuf.len, databuf.buf);
  ri = NULL;

clean:
  free(err);
  mbuf_free(&databuf);

  (void) cb_arg;
  (void) args;
  (void) fi;
}

bool mgos_rpc_service_cron_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();

  mg_rpc_add_handler(c, "Cron.Add", ITEM_FMT, rpc_handler_cron_add_item, NULL);

  mg_rpc_add_handler(c, "Cron.Edit", ITEM_FMT, rpc_handler_cron_edit_item,
                     NULL);

  mg_rpc_add_handler(c, "Cron.Remove", "{id: %d}", rpc_handler_cron_remove_item,
                     NULL);

  mg_rpc_add_handler(c, "Cron.Get", "{id: %d}", rpc_handler_cron_get_item,
                     NULL);

  mg_rpc_add_handler(c, "Cron.List", "", rpc_handler_cron_list_items, NULL);

  return true;
}
