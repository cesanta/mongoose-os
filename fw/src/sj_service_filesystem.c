/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_FILESYSTEM_SERVICE)

#include <stdlib.h>

#include "common/cs_dirent.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mg_str.h"
#include "fw/src/mg_clubby.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_service_filesystem.h"
#include "fw/src/sj_sys_config.h"

#define SJ_FILESYSTEM_LIST_CMD "/v1/Filesystem.List"
#define SJ_FILESYSTEM_GET_CMD "/v1/Filesystem.Get"
#define SJ_FILESYSTEM_PUT_CMD "/v1/Filesystem.Put"

struct put_data {
  char *p;
  size_t len;
};

/* Handler for /v1/Filesystem.List */
static void sj_filesystem_list_handler(struct clubby_request_info *ri,
                                       void *cb_arg,
                                       struct clubby_frame_info *fi,
                                       struct mg_str args) {
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);
  DIR *dirp;

  mbuf_init(&fb, 50);

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
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

  mbuf_free(&fb);

  (void) cb_arg;
  (void) args;
}

static void sj_filesystem_get_handler(struct clubby_request_info *ri,
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
    goto clean;
  }

  json_scanf(args.p, args.len, "{filename: %Q, offset: %ld, len: %ld}",
             &filename, &offset, &len);

  /* check arguments */
  if (filename == NULL) {
    clubby_send_errorf(ri, 400, "filename is required");
    goto clean;
  }

  if (offset < 0) {
    clubby_send_errorf(ri, 400, "illegal offset");
    goto clean;
  }

  /* try to open file */
  fp = fopen(filename, "rb");

  if (fp == NULL) {
    clubby_send_errorf(ri, 400, "failed to open file \"%s\"", filename);
    goto clean;
  }

  /* determine file size */
  if (fseek(fp, 0, SEEK_END) != 0) {
    clubby_send_errorf(ri, 500, "fseek");
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
      goto clean;
    }

    /* seek & read the data */
    if (fseek(fp, offset, SEEK_SET)) {
      clubby_send_errorf(ri, 500, "fseek");
      goto clean;
    }

    if ((long) fread(data, 1, len, fp) != len) {
      clubby_send_errorf(ri, 500, "fread");
      goto clean;
    }
  }

  /* send the response */
  clubby_send_responsef(ri, "{data: %.*Q, left: %d}", len, data,
                        (file_size - offset - len));

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

/*
 * TODO(dfrank): this function needs to be severely improved (and probably
 * rewritten, because implementation smells): currently it only supports
 * special chars like \n, \t, etc; but we need to also support \xNN and \uNNNN.
 *
 * Also, it should be moved to frozen and used internally for `%Q`, as well as
 * exported for clients.
 */
static size_t string_unescape(char *dst, size_t dst_len, const char *src,
                              size_t src_len) {
  size_t len = 0;
  char cur;

  while (src_len-- > 0) {
    if (*src == '\\' && src_len > 0) {
      src_len--;
      src++;
      switch (*src) {
        case 'a':
          cur = '\a';
          break;
        case 'b':
          cur = '\b';
          break;
        case 'f':
          cur = '\f';
          break;
        case 'n':
          cur = '\n';
          break;
        case 'r':
          cur = '\r';
          break;
        case 't':
          cur = '\t';
          break;
        case 'v':
          cur = '\v';
          break;
        case '\\':
          cur = '\\';
          break;
        case '\"':
          cur = '\"';
          break;
        case '\'':
          cur = '\'';
          break;
        default:
          len++;
          if (dst_len > 0) {
            dst_len--;
            *dst++ = '\\';
          }
          cur = *src;
          break;
      }
    } else {
      cur = *src;
    }

    len++;
    if (dst_len > 0) {
      dst_len--;
      *dst++ = cur;
    }

    src++;
  }

  return len;
}

/*
 * TODO(dfrank): when frozen supports `%.*Q` (or whatever specifier will be
 * used for string vector), convert `%M` to it and get rid of this function.
 */
static void data_hnd(const char *str, int len, void *user_data) {
  struct put_data *data = (struct put_data *) user_data;
  size_t unescaped_len = string_unescape(NULL, 0, str, (size_t) len);
  data->p = malloc(unescaped_len);
  data->len = unescaped_len;
  string_unescape(data->p, data->len, str, (size_t) len);
}

static void sj_filesystem_put_handler(struct clubby_request_info *ri,
                                      void *cb_arg,
                                      struct clubby_frame_info *fi,
                                      struct mg_str args) {
  char *filename = NULL;
  int append = 0;
  FILE *fp = NULL;
  struct put_data data = {NULL, 0};

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    goto clean;
  }

  /*
   * TODO(dfrank): when frozen supports `%.*Q` (or whatever specifier will be
   * used for string vector), convert `%M` to it.
   */
  json_scanf(args.p, args.len, "{filename: %Q, data: %M, append: %B}",
             &filename, data_hnd, &data, &append);

  /* check arguments */
  if (filename == NULL) {
    clubby_send_errorf(ri, 400, "filename is required");
    goto clean;
  }

  /* try to open file */
  fp = fopen(filename, append ? "ab" : "wb");

  if (fp == NULL) {
    clubby_send_errorf(ri, 400, "failed to open file \"%s\"", filename);
    goto clean;
  }

  if (fwrite(data.p, 1, data.len, fp) != data.len) {
    clubby_send_errorf(ri, 500, "failed to write data");
    goto clean;
  }

  clubby_send_responsef(ri, NULL);

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

enum sj_init_result sj_service_filesystem_init(void) {
  struct clubby *c = mg_clubby_get_global();
  clubby_add_handler(c, mg_mk_str(SJ_FILESYSTEM_LIST_CMD),
                     sj_filesystem_list_handler, NULL);
  clubby_add_handler(c, mg_mk_str(SJ_FILESYSTEM_GET_CMD),
                     sj_filesystem_get_handler, NULL);
  clubby_add_handler(c, mg_mk_str(SJ_FILESYSTEM_PUT_CMD),
                     sj_filesystem_put_handler, NULL);
  return SJ_INIT_OK;
}

#endif /* SJ_ENABLE_CLUBBY && SJ_ENABLE_FILESYSTEM_SERVICE */
