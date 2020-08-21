/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_file_utils.h"

#include <stdio.h>
#include <string.h>

#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "common/platform.h"

#if defined(SL_MAJOR_VERSION_NUM)
#include "common/platforms/simplelink/sl_fs_slfs.h"
#endif

bool mgos_file_copy(const char *from, const char *to) {
  bool ret = false;
  FILE *from_fp = NULL, *to_fp = NULL;
  int total = 0;

  LOG(LL_INFO, ("Copying %s -> %s", from, to));

  from_fp = fopen(from, "r");
  if (from_fp == NULL) {
    LOG(LL_ERROR, ("Failed to open %s", from));
    goto out;
  }

#if defined(MG_FS_SLFS)
  /* SimpleLink requires advance notice of file size. */
  if (mg_str_starts_with(mg_mk_str(to), mg_mk_str("/slfs/"))) {
    if (fseek(from_fp, 0, SEEK_END) == 0) {
      size_t size = ftell(from_fp);
      fs_slfs_set_file_size(to + 5, size);
      fseek(from_fp, 0, SEEK_SET);
    }
  }
#endif

  to_fp = fopen(to, "w");
  if (to_fp == NULL) {
    LOG(LL_ERROR, ("Failed to open %s", to));
    goto out;
  }

  char buf[128];
  while (true) {
    int n = fread(buf, 1, sizeof(buf), from_fp);
    if (n < 0) {
      LOG(LL_ERROR, ("Failed to read from %s", from));
      goto out;
    }
    if (fwrite(buf, 1, n, to_fp) != (size_t) n) {
      LOG(LL_ERROR, ("Failed to write %d bytes to %s", n, to));
      goto out;
    }
    total += n;
    if (n < (int) sizeof(buf)) break;
  }

  LOG(LL_DEBUG, ("Wrote %d to %s", total, to));

  ret = true;

out:
  if (from_fp != NULL) fclose(from_fp);
  if (to_fp != NULL) {
    fclose(to_fp);
    if (!ret) remove(to);
  }
#if defined(MG_FS_SLFS)
  if (mg_str_starts_with(mg_mk_str(to), mg_mk_str("/slfs/"))) {
    fs_slfs_unset_file_flags(to + 5);
  }
#endif
  return ret;
}

#ifdef MGOS_HAVE_MBEDTLS
bool mgos_file_digest(const char *fname, mbedtls_md_type_t dt,
                      uint8_t *digest) {
  bool res = false;
  FILE *fp = NULL;
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *di;

  mbedtls_md_init(&ctx);
  di = mbedtls_md_info_from_type(dt);
  if (di == NULL) goto out;

  fp = fopen(fname, "r");
  if (fp == NULL) goto out;

  unsigned char buf[128];
  if (mbedtls_md_setup(&ctx, di, false /* hmac */) != 0) goto out;
  if (mbedtls_md_starts(&ctx) != 0) goto out;
  while (true) {
    int n = fread(buf, 1, sizeof(buf), fp);
    if (n < 0) goto out;
    if (mbedtls_md_update(&ctx, buf, n) != 0) goto out;
    if (n < (int) sizeof(buf)) break;
  }
  if (mbedtls_md_finish(&ctx, digest) != 0) goto out;
  res = true;

out:
  if (fp != NULL) fclose(fp);
  mbedtls_md_free(&ctx);
  return res;
}

bool mgos_file_copy_if_different(const char *from, const char *to) {
  bool res = false;
  uint8_t df[32], dt[32];
  if (!mgos_file_digest(from, MBEDTLS_MD_SHA256, df)) goto out;
  if (!mgos_file_digest(to, MBEDTLS_MD_SHA256, dt)) goto out;
  res = (memcmp(df, dt, sizeof(df)) == 0);
  if (res) {
    LOG(LL_DEBUG, ("%s and %s are the same", from, to));
  }

out:
  if (!res) res = mgos_file_copy(from, to);
  return res;
}
#endif  // MGOS_HAVE_MBEDTLS
