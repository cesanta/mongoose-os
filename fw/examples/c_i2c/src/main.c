#include <stdio.h>

#include "mgos_http_server.h"
#include "mgos_i2c.h"

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_utils.h"

static uint8_t from_hex(const char *s) {
#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')
  int a = tolower(*(const unsigned char *) s);
  int b = tolower(*(const unsigned char *) (s + 1));
  return (HEXTOI(a) << 4) | HEXTOI(b);
}

static void i2c_handler(struct mg_connection *c, int ev, void *p,
                        void *user_data) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *) p;
    struct mg_str *s = hm->body.len > 0 ? &hm->body : &hm->query_string;
    bool ret = false;

    LOG(LL_INFO, ("Got: [%.*s]", (int) s->len, s->p));

    if (s->len >= 2) {
      int j = 0;
      struct mgos_i2c *i2c = mgos_i2c_get_global();
      for (size_t i = 0; i < s->len; i += 2, j++) {
        ((uint8_t *) s->p)[j] = from_hex(s->p + i);
      }
      ret = mgos_i2c_write(i2c, s->p[0], s->p + 1, j, true /* stop */);
    }

    mg_send_head(c, 200, -1, NULL);  // Use chunked encoding
    mg_printf_http_chunk(c, "{\"status\": %d}\n", ret);
    mg_printf_http_chunk(c, "");  // Zero chunk, end of output
  }
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_register_http_endpoint("/i2c", i2c_handler, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
