/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MGOS_ENABLE_RPC && MGOS_ENABLE_FILESYSTEM_SERVICE

#include <stdlib.h>

#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mg_str.h"
#include "fw/src/mgos_config.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_service_filesystem.h"
#include "fw/src/mgos_sys_config.h"

#if MG_ENABLE_DIRECTORY_LISTING

#if !defined(CS_DEFINE_DIRENT)
#include <dirent.h>
#else
#include "common/cs_dirent.h"
#endif

/* Handler for FS.List */
static void mgos_fs_list_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);
  DIR *dirp;

  mbuf_init(&fb, 50);

  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  json_printf(&out, "[");

  if ((dirp = (opendir("." /*not really used by SPIFFS*/))) != NULL) {
    struct dirent *dp;
    int i;
    for (i = 0; (dp = readdir(dirp)) != NULL; i++) {
      /* Do not show current and parent dirs */
      if (strcmp((const char *) dp->d_name, ".") == 0 ||
          strcmp((const char *) dp->d_name, "..") == 0) {
        continue;
      }

      if (i > 0) {
        json_printf(&out, ",");
      }

      json_printf(&out, "%Q", dp->d_name);
    }
    closedir(dirp);
  }

  json_printf(&out, "]");

  mg_rpc_send_responsef(ri, "%.*s", fb.len, fb.buf);
  ri = NULL;

  mbuf_free(&fb);

  (void) cb_arg;
  (void) args;
}
#endif /* MG_ENABLE_DIRECTORY_LISTING */

static void mgos_fs_get_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  char *filename = NULL;
  long offset = 0, len = -1;
  long file_size = 0;
  FILE *fp = NULL;
  char *data = NULL;

  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    goto clean;
  }

  json_scanf(args.p, args.len, ri->args_fmt, &filename, &offset, &len);

  /* check arguments */
  if (filename == NULL) {
    mg_rpc_send_errorf(ri, 400, "filename is required");
    ri = NULL;
    goto clean;
  }

  if (offset < 0) {
    mg_rpc_send_errorf(ri, 400, "illegal offset");
    ri = NULL;
    goto clean;
  }

  /* try to open file */
  fp = fopen(filename, "rb");

  if (fp == NULL) {
    mg_rpc_send_errorf(ri, 400, "failed to open file \"%s\"", filename);
    ri = NULL;
    goto clean;
  }

  /* determine file size */
  cs_stat_t st;
  if (mg_stat(filename, &st) != 0) {
    mg_rpc_send_errorf(ri, 500, "stat");
    ri = NULL;
    goto clean;
  }

  file_size = (long) st.st_size;

  /* determine the size of the chunk to read */
  if (offset > file_size) {
    offset = file_size;
  }
  if (len < 0 || offset + len > file_size) {
    len = file_size - offset;
  }

  if (len > 0) {
    /* try to allocate the chunk of needed size */
    data = (char *) malloc(len);
    if (data == NULL) {
      mg_rpc_send_errorf(ri, 500, "out of memory");
      ri = NULL;
      goto clean;
    }

    if (offset == 0) {
      LOG(LL_INFO, ("Sending %s", filename));
    }

    /* seek & read the data */
    if (fseek(fp, offset, SEEK_SET)) {
      mg_rpc_send_errorf(ri, 500, "fseek");
      ri = NULL;
      goto clean;
    }

    if ((long) fread(data, 1, len, fp) != len) {
      mg_rpc_send_errorf(ri, 500, "fread");
      ri = NULL;
      goto clean;
    }
  }

  /* send the response */
  mg_rpc_send_responsef(ri, "{data: %V, left: %d}", data, len,
                        (file_size - offset - len));
  ri = NULL;

clean:
  if (filename != NULL) {
    free(filename);
  }

  if (data != NULL) {
    free(data);
  }

  if (fp != NULL) {
    fclose(fp);
  }

  (void) cb_arg;
}

struct put_data {
  char *p;
  int len;
};

static void mgos_fs_put_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  char *filename = NULL;
  int append = 0;
  FILE *fp = NULL;
  struct put_data data = {NULL, 0};

  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    goto clean;
  }

  json_scanf(args.p, args.len, ri->args_fmt, &filename, &data.p, &data.len,
             &append);

  /* check arguments */
  if (filename == NULL) {
    mg_rpc_send_errorf(ri, 400, "filename is required");
    ri = NULL;
    goto clean;
  }

  /* try to open file */
  fp = fopen(filename, append ? "ab" : "wb");

  if (fp == NULL) {
    mg_rpc_send_errorf(ri, 400, "failed to open file \"%s\"", filename);
    ri = NULL;
    goto clean;
  }

  if (!append) {
    LOG(LL_INFO, ("Receiving %s", filename));
  }

  if (fwrite(data.p, 1, data.len, fp) != (size_t) data.len) {
    mg_rpc_send_errorf(ri, 500, "failed to write data");
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

clean:
  if (filename != NULL) {
    free(filename);
  }

  if (data.p != NULL) {
    free(data.p);
  }

  if (fp != NULL) {
    fclose(fp);
  }

  (void) cb_arg;
}

static void mgos_fs_remove_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                   struct mg_rpc_frame_info *fi,
                                   struct mg_str args) {
  int ret = 0;
  char *filename = NULL;

  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    goto clean;
  }

  json_scanf(args.p, args.len, ri->args_fmt, &filename);

  /* check arguments */
  if (filename == NULL) {
    mg_rpc_send_errorf(ri, 400, "filename is required");
    ri = NULL;
    goto clean;
  }

  ret = remove(filename);
  LOG(LL_INFO, ("Remove %s -> %d", filename, ret));
  if (ret != 0) {
    mg_rpc_send_errorf(ri, 500, "remove failed");
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

clean:
  if (filename != NULL) {
    free(filename);
  }

  (void) cb_arg;
}

enum mgos_init_result mgos_service_filesystem_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
#if MG_ENABLE_DIRECTORY_LISTING
  mg_rpc_add_handler(c, "FS.List", "", mgos_fs_list_handler, NULL);
#endif
  mg_rpc_add_handler(c, "FS.Get", "{filename: %Q, offset: %ld, len: %ld}",
                     mgos_fs_get_handler, NULL);
  mg_rpc_add_handler(c, "FS.Put", "{filename: %Q, data: %V, append: %B}",
                     mgos_fs_put_handler, NULL);
  mg_rpc_add_handler(c, "FS.Remove", "{filename: %Q}", mgos_fs_remove_handler,
                     NULL);
  return MGOS_INIT_OK;
}

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_FILESYSTEM_SERVICE */
