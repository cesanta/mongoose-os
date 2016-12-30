#include <stdio.h>

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_i2c.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_utils.h"

static uint8_t from_hex(const char *s) {
#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')
  int a = tolower(*(const unsigned char *) s);
  int b = tolower(*(const unsigned char *) (s + 1));
  return (HEXTOI(a) << 4) | HEXTOI(b);
}

static void i2c_handler(struct mg_connection *c, int ev, void *p) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *) p;
    struct mg_str *s = hm->body.len > 0 ? &hm->body : &hm->query_string;
    enum i2c_ack_type ret = I2C_ACK;

    LOG(LL_INFO, ("Got: [%.*s]", (int) s->len, s->p));

    if (s->len >= 2) {
      struct mgos_i2c *i2c = mgos_i2c_get_global();
      ret = mgos_i2c_start(i2c, from_hex(s->p), I2C_WRITE);
      for (size_t i = 2; i < s->len && ret == I2C_ACK; i += 2) {
        ret = mgos_i2c_send_byte(i2c, from_hex(s->p + i));
        LOG(LL_DEBUG, ("i2c -> %02x", from_hex(s->p + i)));
      }
      mgos_i2c_stop(i2c);
    }

    mg_send_head(c, 200, -1, NULL);  // Use chunked encoding
    mg_printf_http_chunk(c, "{\"status\": %d}\n", ret);
    mg_printf_http_chunk(c, "");  // Zero chunk, end of output
  }
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_register_http_endpoint("/i2c", i2c_handler);
  return MGOS_APP_INIT_SUCCESS;
}
