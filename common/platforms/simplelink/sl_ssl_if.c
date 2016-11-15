/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MG_ENABLE_SSL && MG_SSL_IF == MG_SSL_IF_SIMPLELINK

struct mg_ssl_if_ctx {
  char *ssl_cert;
  char *ssl_key;
  char *ssl_ca_cert;
  char *ssl_server_name;
};

void mg_ssl_if_init() {
}

enum mg_ssl_if_result mg_ssl_if_conn_init(
    struct mg_connection *nc, const struct mg_ssl_if_conn_params *params,
    const char **err_msg) {
  struct mg_ssl_if_ctx *ctx =
      (struct mg_ssl_if_ctx *) MG_CALLOC(1, sizeof(*ctx));
  if (ctx == NULL) {
    MG_SET_PTRPTR(err_msg, "Out of memory");
    return MG_SSL_ERROR;
  }
  nc->ssl_if_data = ctx;

  if (params->cert != NULL || params->key != NULL) {
    if (params->cert != NULL && params->key != NULL) {
      ctx->ssl_cert = strdup(params->cert);
      ctx->ssl_key = strdup(params->key);
    } else {
      MG_SET_PTRPTR(err_msg, "Both cert and key are required.");
      return MG_SSL_ERROR;
    }
  }
  if (params->ca_cert != NULL && strcmp(params->ca_cert, "*") != 0) {
    ctx->ssl_ca_cert = strdup(params->ca_cert);
  }
  if (params->server_name != NULL) {
    ctx->ssl_server_name = strdup(params->server_name);
  }
  return MG_SSL_OK;
}

void mg_ssl_if_conn_free(struct mg_connection *nc) {
  struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
  if (ctx == NULL) return;
  nc->ssl_if_data = NULL;
  MG_FREE(ctx->ssl_cert);
  MG_FREE(ctx->ssl_key);
  MG_FREE(ctx->ssl_ca_cert);
  MG_FREE(ctx->ssl_server_name);
  memset(ctx, 0, sizeof(*ctx));
  MG_FREE(ctx);
}

int sl_set_ssl_opts(struct mg_connection *nc) {
  int err;
  struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
  DBG(("%p ssl ctx: %p", nc, ctx));

  if (ctx) {
    DBG(("%p %s,%s,%s,%s", nc, (ctx->ssl_cert ? ctx->ssl_cert : "-"),
         (ctx->ssl_key ? ctx->ssl_cert : "-"),
         (ctx->ssl_ca_cert ? ctx->ssl_ca_cert : "-"),
         (ctx->ssl_server_name ? ctx->ssl_server_name : "-")));
    if (ctx->ssl_cert != NULL && ctx->ssl_key != NULL) {
      err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                          SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
                          ctx->ssl_cert, strlen(ctx->ssl_cert));
      DBG(("CERTIFICATE_FILE_NAME %s -> %d", ctx->ssl_cert, err));
      if (err != 0) return err;
      err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                          SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
                          ctx->ssl_key, strlen(ctx->ssl_key));
      DBG(("PRIVATE_KEY_FILE_NAME %s -> %d", ctx->ssl_key, nc->err));
      if (err != 0) return err;
    }
    if (ctx->ssl_ca_cert != NULL) {
      if (ctx->ssl_ca_cert[0] != '\0') {
        err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                            SL_SO_SECURE_FILES_CA_FILE_NAME, ctx->ssl_ca_cert,
                            strlen(ctx->ssl_ca_cert));
        DBG(("CA_FILE_NAME %s -> %d", ctx->ssl_ca_cert, err));
        if (err != 0) return err;
      }
    }
    if (ctx->ssl_server_name != NULL) {
      err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                          SO_SECURE_DOMAIN_NAME_VERIFICATION,
                          ctx->ssl_server_name, strlen(ctx->ssl_server_name));
      DBG(("DOMAIN_NAME_VERIFICATION %s -> %d", ctx->ssl_server_name, err));
      /* Domain name verificationw as added in a NWP service pack, older
       * versions
       * return SL_ENOPROTOOPT. There isn't much we can do about it, so we
       * ignore
       * the error. */
      if (err != 0 && err != SL_ENOPROTOOPT) return err;
    }
  }
  return 0;
}

#endif /* MG_ENABLE_SSL && MG_SSL_IF == MG_SSL_IF_SIMPLELINK */
