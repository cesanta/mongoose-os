/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MG_ENABLE_CLUBBY && MG_ENABLE_FILESYSTEM_SERVICE

#include <stdlib.h>

#include "common/cs_dirent.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mg_str.h"
#include "fw/src/mg_clubby.h"
#include "fw/src/mg_config.h"
#include "fw/src/mg_service_filesystem.h"
#include "fw/src/mg_sys_config.h"

#define MG_FILESYSTEM_LIST_CMD "/v1/Filesystem.List"
#define MG_FILESYSTEM_GET_CMD "/v1/Filesystem.Get"
#define MG_FILESYSTEM_PUT_CMD "/v1/Filesystem.Put"

struct put_data {
  char *p;
  int len;
};

/* Handler for /v1/Filesystem.List */
static void mg_filesystem_list_handler(struct clubby_request_info *ri,
                                       void *cb_arg,
                                       struct clubby_frame_info *fi,
                                       struct mg_str args) {
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);
  DIR *dirp;

  mbuf_init(&fb, 50);

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
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

  clubby_send_responsef(ri, "%.*s", fb.len, fb.buf);
  ri = NULL;

  mbuf_free(&fb);

  (void) cb_arg;
  (void) args;
}

static void mg_filesystem_get_handler(struct clubby_request_info *ri,
                                      void *cb_arg,
                                      struct clubby_frame_info *fi,
                                      struct mg_str args) {
  char *filename = NULL;
  long offset = 0, len = -1;
  long file_size = 0;
  FILE *fp = NULL;
  char *data = NULL;

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    goto clean;
  }

  json_scanf(args.p, args.len, "{filename: %Q, offset: %ld, len: %ld}",
             &filename, &offset, &len);

  /* check arguments */
  if (filename == NULL) {
    clubby_send_errorf(ri, 400, "filename is required");
    ri = NULL;
    goto clean;
  }

  if (offset < 0) {
    clubby_send_errorf(ri, 400, "illegal offset");
    ri = NULL;
    goto clean;
  }

  /* try to open file */
  fp = fopen(filename, "rb");

  if (fp == NULL) {
    clubby_send_errorf(ri, 400, "failed to open file \"%s\"", filename);
    ri = NULL;
    goto clean;
  }

  /* determine file size */
  if (fseek(fp, 0, SEEK_END) != 0) {
    clubby_send_errorf(ri, 500, "fseek");
    ri = NULL;
    goto clean;
  }

  file_size = (long) ftell(fp);

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
      clubby_send_errorf(ri, 500, "out of memory");
      ri = NULL;
      goto clean;
    }

    /* seek & read the data */
    if (fseek(fp, offset, SEEK_SET)) {
      clubby_send_errorf(ri, 500, "fseek");
      ri = NULL;
      goto clean;
    }

    if ((long) fread(data, 1, len, fp) != len) {
      clubby_send_errorf(ri, 500, "fread");
      ri = NULL;
      goto clean;
    }
  }

  /* send the response */
  clubby_send_responsef(ri, "{data: %V, left: %d}", data, len,
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

static void mg_filesystem_put_handler(struct clubby_request_info *ri,
                                      void *cb_arg,
                                      struct clubby_frame_info *fi,
                                      struct mg_str args) {
  char *filename = NULL;
  int append = 0;
  FILE *fp = NULL;
  struct put_data data = {NULL, 0};

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    goto clean;
  }

  json_scanf(args.p, args.len, "{filename: %Q, data: %V, append: %B}",
             &filename, &data.p, &data.len, &append);

  /* check arguments */
  if (filename == NULL) {
    clubby_send_errorf(ri, 400, "filename is required");
    ri = NULL;
    goto clean;
  }

  /* try to open file */
  fp = fopen(filename, append ? "ab" : "wb");

  if (fp == NULL) {
    clubby_send_errorf(ri, 400, "failed to open file \"%s\"", filename);
    ri = NULL;
    goto clean;
  }

  if (fwrite(data.p, 1, data.len, fp) != (size_t) data.len) {
    clubby_send_errorf(ri, 500, "failed to write data");
    ri = NULL;
    goto clean;
  }

  clubby_send_responsef(ri, NULL);
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

enum mg_init_result mg_service_filesystem_init(void) {
  struct clubby *c = mg_clubby_get_global();
  clubby_add_handler(c, mg_mk_str(MG_FILESYSTEM_LIST_CMD),
                     mg_filesystem_list_handler, NULL);
  clubby_add_handler(c, mg_mk_str(MG_FILESYSTEM_GET_CMD),
                     mg_filesystem_get_handler, NULL);
  clubby_add_handler(c, mg_mk_str(MG_FILESYSTEM_PUT_CMD),
                     mg_filesystem_put_handler, NULL);
  return MG_INIT_OK;
}

#endif /* MG_ENABLE_CLUBBY && MG_ENABLE_FILESYSTEM_SERVICE */
