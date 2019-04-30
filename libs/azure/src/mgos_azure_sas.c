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

#include "mgos_azure.h"

#include <stdint.h>
#include <stdio.h>

#include "common/cs_base64.h"
#include "common/cs_dbg.h"
#include "common/mg_str.h"

#include "mongoose.h"

#include "mbedtls/md.h"

struct mg_str mgos_azure_gen_sas_token(const struct mg_str uri,
                                       const struct mg_str key, uint64_t se) {
  char buf[32], b64buf[45];
  const struct mg_str noq = mg_mk_str_n("._-$,;~()", 9);
  struct mg_str res = MG_NULL_STR;
  struct mg_str sr = MG_NULL_STR, sig = MG_NULL_STR;
  struct mg_str s = MG_NULL_STR, k = mg_strdup(key);
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  int len = 0;
  if (cs_base64_decode((const unsigned char *) key.p, (int) key.len,
                       (char *) k.p, &len) != (int) key.len) {
    LOG(LL_ERROR, ("Invalid signing key"));
    goto out;
  }
  k.len = len;
  if (se < 1500000000) {
    LOG(LL_ERROR, ("Time is not set, Azure connection will fail. "
                   "Set the time or make sure SNTP is enabled and working."));
  }
  len = snprintf(buf, sizeof(buf), "%llu", (long long unsigned int) se);
  if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                       1 /* hmac */) != 0) {
    goto out;
  }
  if (mbedtls_md_hmac_starts(&ctx, (uint8_t *) k.p, k.len) != 0) {
    goto out;
  }
  sr = mg_url_encode_opt(uri, noq, MG_URL_ENCODE_F_UPPERCASE_HEX);
  if (mbedtls_md_hmac_update(&ctx, (uint8_t *) sr.p, sr.len) != 0 ||
      mbedtls_md_hmac_update(&ctx, (uint8_t *) "\n", 1) != 0 ||
      mbedtls_md_hmac_update(&ctx, (uint8_t *) buf, len) != 0) {
    goto out;
  }
  if (mbedtls_md_hmac_finish(&ctx, (uint8_t *) buf) != 0) {
    goto out;
  }
  mbedtls_md_free(&ctx);
  cs_base64_encode((uint8_t *) buf, sizeof(buf), b64buf);
  sig = mg_url_encode_opt(mg_mk_str_n(b64buf, 44), noq,
                          MG_URL_ENCODE_F_UPPERCASE_HEX);
  res.len = mg_asprintf(
      (char **) &res.p, 0, "SharedAccessSignature sr=%.*s&sig=%.*s&se=%llu",
      (int) sr.len, sr.p, (int) sig.len, sig.p, (long long unsigned int) se);
  if (res.p == NULL) {
    res.len = 0;
  }

out:
  mbedtls_md_free(&ctx);
  free((void *) sr.p);
  free((void *) sig.p);
  free((void *) s.p);
  free((void *) k.p);
  return res;
}
