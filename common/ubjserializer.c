/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <assert.h>

#include "common/ubjson.h"
#include "common/ubjserializer.h"
#include "common/cs_dbg.h"

#if CS_ENABLE_UBJSON

struct link {
  struct link *next;
  /* arrays need a finalizer; don't bother using tagged pointers */
  int is_array;
};

struct prop {
  struct link link; /* for freeing up */
  struct prop *next;
  struct ub_str *name;
  ub_val_t val;
};

struct ub_obj {
  struct link link;
  struct prop *props;
};

struct ub_arr {
  struct link link;
  struct mbuf elems;
};

struct ub_str {
  struct link link;
  char s[1];
};

struct ub_bin {
  struct link link; /* for freeing up */
  size_t size;
  ub_bin_cb_t cb;
  void *user_data;
};

/* rendering */

struct ub_ctx {
  struct mbuf out;   /* output buffer */
  struct mbuf stack; /* visit stack */
  ub_cb_t cb;        /* called to render data */
  void *user_data;   /* passed to cb */
  size_t bytes_left; /* bytes left in current Bin generator */
  struct link *head; /* all allocated objects */
};

struct visit {
  ub_val_t obj;
  union {
    size_t next_idx;
    struct prop *p;
  } v;
};

static void ub_call_cb(struct ub_ctx *ctx, int end);
static size_t ub_array_length(ub_val_t);
static ub_val_t ub_array_get(ub_val_t, size_t);
static ub_val_t ub_get(ub_val_t, const char *name);

static struct visit *push_visit(struct mbuf *stack, ub_val_t obj) {
  struct visit *res;
  size_t pos = stack->len;
  mbuf_append(stack, NULL, sizeof(struct visit));
  res = (struct visit *) (stack->buf + pos);
  memset(res, 0, sizeof(struct visit));
  res->obj = obj;
  return res;
}

static struct visit *cur_visit(struct mbuf *stack) {
  if (stack->len == 0) return NULL;
  return (struct visit *) (stack->buf + stack->len - sizeof(struct visit));
}

static void pop_visit(struct mbuf *stack) {
  stack->len -= sizeof(struct visit);
}

struct ub_ctx *ub_ctx_new(void) {
  struct ub_ctx *ctx = (struct ub_ctx *) calloc(1, sizeof(*ctx));
  mbuf_init(&ctx->out, 0);
  mbuf_init(&ctx->stack, 0);
  return ctx;
}

void ub_ctx_free(struct ub_ctx *ctx) {
  mbuf_free(&ctx->out);
  mbuf_free(&ctx->stack);
  struct link *l, *tmp;
  for (l = ctx->head; l != NULL; l = tmp) {
    tmp = l->next;
    if (l->is_array) {
      /* l points to the beginning of the array object */
      mbuf_free(&((struct ub_arr *) l)->elems);
    }
    free(l);
  }
  free(ctx);
}

struct prop *ub_next_prop(ub_val_t obj, struct prop *p) {
  assert(obj.kind == UBJSON_TYPE_OBJECT);
  return p == NULL ? obj.val.o->props : p->next;
}

/* This will be called many time to advance rendering of an ubjson ctx */
static void ub_render_cont(struct ub_ctx *ctx) {
  struct mbuf *buf = &ctx->out, *stack = &ctx->stack;
  struct visit *cur;

  if (ctx->out.len > 0) {
    ub_call_cb(ctx, 0);
  }

  for (cur = cur_visit(stack); cur != NULL; cur = cur_visit(stack)) {
    ub_val_t obj = cur->obj;

    if (obj.kind == UBJSON_TYPE_NULL) {
      cs_ubjson_emit_null(buf);
    } else if (obj.kind == UBJSON_TYPE_TRUE) {
      cs_ubjson_emit_boolean(buf, 1);
    } else if (obj.kind == UBJSON_TYPE_FALSE) {
      cs_ubjson_emit_boolean(buf, 0);
    } else if (obj.kind == UBJSON_TYPE_NUMBER) {
      cs_ubjson_emit_autonumber(buf, obj.val.n);
    } else if (obj.kind == UBJSON_TYPE_STRING) {
      cs_ubjson_emit_string(buf, obj.val.s->s, strlen(obj.val.s->s));
    } else if (obj.kind == UBJSON_TYPE_ARRAY) {
      unsigned long cur_idx = cur->v.next_idx;

      if (cur->v.next_idx == 0) {
        cs_ubjson_open_array(buf);
      }

      cur->v.next_idx++;

      if (cur->v.next_idx > ub_array_length(cur->obj)) {
        cs_ubjson_close_array(buf);
      } else {
        cur = push_visit(stack, ub_array_get(obj, cur_idx));
        /* skip default popping of visitor frame */
        continue;
      }
    } else if (obj.kind == UBJSON_TYPE_BIN) {
      ctx->bytes_left = obj.val.b->size;
      cs_ubjson_emit_bin_header(buf, ctx->bytes_left);
      pop_visit(stack);

      obj.val.b->cb(ctx, obj.val.b->user_data);
      /*
       * The user generator will reenter calling this function again with the
       * same context.
       */
      return;

    } else if (obj.kind == UBJSON_TYPE_OBJECT) {
      const char *s;

      if (cur->v.p == NULL) {
        cs_ubjson_open_object(buf);
      }

      cur->v.p = ub_next_prop(obj, cur->v.p);

      if (cur->v.p == NULL) {
        cs_ubjson_close_object(buf);
      } else {
        s = cur->v.p->name->s;
        cs_ubjson_emit_object_key(buf, s, strlen(s));

        cur = push_visit(stack, ub_get(obj, s));
        /* skip default popping of visitor frame */
        continue;
      }
    } else {
      printf("ubsjon: unsupported object: %d\n", obj.kind);
    }

    pop_visit(stack);
  }

  ub_call_cb(ctx, 1);
  mbuf_free(&ctx->out);
  ub_ctx_free(ctx);
}

void ub_render(struct ub_ctx *ctx, ub_val_t root, ub_cb_t cb, void *user_data) {
  ctx->cb = cb;
  ctx->user_data = user_data;
  push_visit(&ctx->stack, root);
  ub_render_cont(ctx);
}

void ub_bin_send(struct ub_ctx *ctx, void *d, size_t n) {
  if (n > ctx->bytes_left) {
    n = ctx->bytes_left;
  }
  ctx->bytes_left -= n;

  /*
   * TODO(mkm):
   * this is useless buffering, we should call ubjson cb directly
   */
  mbuf_append(&ctx->out, d, n);
  ub_call_cb(ctx, 0);

  if (ctx->bytes_left == 0) {
    ub_render_cont(ctx);
  }
}

static void *ub_alloc(struct ub_ctx *ctx, size_t size) {
  struct link *res = calloc(1, size);
  res->next = ctx->head;
  ctx->head = res;
  return res;
}

struct ub_str *create_ub_str(struct ub_ctx *ctx, const struct mg_str s) {
  struct ub_str *res = ub_alloc(ctx, sizeof(*res) + s.len);
  memcpy((char *) res->s, s.p, s.len + 1);
  return res;
}

ub_val_t ub_create_object(struct ub_ctx *ctx) {
  struct ub_obj *o = ub_alloc(ctx, sizeof(*o));
  ub_val_t res = {UBJSON_TYPE_OBJECT, {.o = o}};
  return res;
}

ub_val_t ub_create_array(struct ub_ctx *ctx) {
  struct ub_arr *a = ub_alloc(ctx, sizeof(*a));
  a->link.is_array = 1;
  mbuf_init(&a->elems, 0);
  ub_val_t res = {UBJSON_TYPE_ARRAY, {.a = a}};
  return res;
}

ub_val_t ub_create_bin(struct ub_ctx *ctx, size_t n, ub_bin_cb_t cb,
                       void *user_data) {
  struct ub_bin *b = ub_alloc(ctx, sizeof(*b));
  b->size = n;
  b->cb = cb;
  b->user_data = user_data;
  ub_val_t res = {UBJSON_TYPE_BIN, {.b = b}};
  return res;
}

ub_val_t ub_create_number(double n) {
  ub_val_t res = {UBJSON_TYPE_NUMBER, {.n = n}};
  return res;
}

ub_val_t ub_create_boolean(int n) {
  ub_val_t res = {n ? UBJSON_TYPE_TRUE : UBJSON_TYPE_FALSE, {}};
  return res;
}

ub_val_t ub_create_null(void) {
  ub_val_t res;
  res.kind = UBJSON_TYPE_NULL;
  return res;
}

ub_val_t ub_create_string(struct ub_ctx *ctx, const struct mg_str str) {
  struct ub_str *s = create_ub_str(ctx, str);
  ub_val_t res = {UBJSON_TYPE_STRING, {.s = s}};
  return res;
}

ub_val_t ub_create_cstring(struct ub_ctx *ctx, const char *s) {
  return ub_create_string(ctx, mg_mk_str(s));
}

ub_val_t ub_create_undefined(void) {
  ub_val_t res = {UBJSON_TYPE_UNDEFINED, {}};
  return res;
}

int ub_is_bin(ub_val_t v) {
  return v.kind == UBJSON_TYPE_BIN;
}

int ub_is_undefined(ub_val_t v) {
  return v.kind == UBJSON_TYPE_UNDEFINED;
}

ub_val_t ub_get(ub_val_t o, const char *name) {
  assert(o.kind == UBJSON_TYPE_OBJECT);
  struct prop *p;
  for (p = o.val.o->props; p != NULL; p = p->next) {
    if (strcmp(p->name->s, name) == 0) {
      return p->val;
    }
  }
  return ub_create_null();
}

void ub_add_prop(struct ub_ctx *ctx, ub_val_t obj, const char *name,
                 ub_val_t val) {
  assert(obj.kind == UBJSON_TYPE_OBJECT);
  struct prop *p = ub_alloc(ctx, sizeof(*p));
  p->next = obj.val.o->props;
  p->name = create_ub_str(ctx, mg_mk_str(name));
  p->val = val;
  obj.val.o->props = p;
}

static size_t ub_array_length(ub_val_t o) {
  assert(o.kind == UBJSON_TYPE_ARRAY);
  return o.val.a->elems.len / sizeof(ub_val_t);
}

static ub_val_t ub_array_get(ub_val_t a, size_t idx) {
  assert(a.kind == UBJSON_TYPE_ARRAY);
  assert((idx * sizeof(ub_val_t)) < a.val.a->elems.len);
  ub_val_t res;
  memcpy(&res, a.val.a->elems.buf + (idx * sizeof(ub_val_t)), sizeof(ub_val_t));
  return res;
}

void ub_array_push(struct ub_ctx *ctx, ub_val_t a, ub_val_t val) {
  (void) ctx; /* for uniformity */
  assert(a.kind == UBJSON_TYPE_ARRAY);
  mbuf_append(&a.val.a->elems, &val, sizeof(ub_val_t));
}

static void ub_call_cb(struct ub_ctx *ctx, int end) {
  if (end || ctx->out.len > 0) {
    size_t len = ctx->out.len;
    ctx->out.len = 0;
    ctx->cb((char *) ctx->out.buf, len, end, ctx->user_data);
  } /* avoid calling cb with no output if we're not done */
}

#ifdef UBJ_MAIN /* test */

static void demo_emit(char *d, size_t l, int end, void *user_data) {
  dbg_printf(LL_DEBUG, "DEMO EMIT size=%lu end=%d\n", l, end);

  (void) user_data;
  if (!end) {
    printf("%.*s", (int) l, d);
  } else {
    dbg_printf(LL_DEBUG, "done\n");
  }
}

static void demo_bin(struct ub_ctx *ctx, void *user_data) {
  /* remaining two bytes will be sent after render exists */
  ub_bin_send(ctx, "de", 2);
}

int main(void) {
  struct ub_ctx *ctx = ub_ctx_new();

  ub_val_t root = ub_create_object(ctx);
  ub_val_t cmds = ub_create_array(ctx);
  ub_add_prop(ctx, root, "src", ub_create_string("//foo"));
  ub_add_prop(ctx, root, "dst", ub_create_string("//bar"));
  ub_add_prop(ctx, root, "cmds", cmds);
  ub_val_t cmd = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmd);
  ub_add_prop(ctx, cmd, "cmd", ub_create_string("/v1/Blob.Set"));
  ub_val_t args = ub_create_object(ctx);
  ub_add_prop(ctx, cmd, "args", args);
  ub_val_t key = ub_create_array(ctx);
  ub_add_prop(ctx, args, "key", key);
  ub_array_push(ctx, key, ub_create_string("//foo"));
  ub_array_push(ctx, key, ub_create_string("stuff"));
  ub_val_t bin = ub_create_bin(ctx, 4, demo_bin, NULL);
  ub_add_prop(ctx, args, "value", bin);

  ub_render(ctx, root, demo_emit, NULL);

  ub_bin_send(ctx, "mox", 3);
}
#endif

#endif /* CS_ENABLE_UBJSON */
