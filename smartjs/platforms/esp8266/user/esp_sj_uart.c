#include "smartjs/platforms/esp8266/user/esp_sj_uart.h"

#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "common/cs_dbg.h"
#include "common/platforms/esp8266/esp_uart.h"
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/esp_mg_net_if.h"

#include "v7/v7.h"
#include "smartjs/src/sj_hal.h"
#include "smartjs/src/sj_prompt.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct esp_sj_uart_state {
  unsigned int uart_no : 1; /* We only have two UARTs, save some space. */
  unsigned int dispatch_pending : 1;
  unsigned int recv_pending : 1;
  unsigned int prompt : 1;

  os_timer_t timer;

  struct v7 *v7;
  v7_val_t obj;
};

static struct esp_sj_uart_state sj_us[2];

static IRAM void esp_sj_uart_schedule_dispatcher(int uart_no) {
  struct esp_sj_uart_state *us = &sj_us[uart_no];
  if (us == NULL || us->dispatch_pending) return;
  us->dispatch_pending = 1;
  os_timer_arm(&us->timer, 0 /* immediately */, 0 /* no repeat */);
}

IRAM void esp_uart_dispatch_signal_from_isr(int uart_no) {
  esp_sj_uart_schedule_dispatcher(uart_no);
}

void esp_sj_uart_dispatcher(void *arg) {
  int uart_no = (int) arg;
  struct esp_sj_uart_state *us = &sj_us[uart_no];
  cs_rbuf_t *rxb = esp_uart_rx_buf(uart_no);
  cs_rbuf_t *txb = esp_uart_tx_buf(uart_no);
  us->dispatch_pending = 0;
  esp_uart_dispatch_rx_top(uart_no);
  uint16_t tx_used_before = txb->used;
  esp_uart_dispatch_tx_top(uart_no);
  if (us->v7 != NULL) {
    /* Detect the edge of TX buffer becoming empty. */
    if (tx_used_before > 0 && txb->used == 0) {
      v7_val_t txcb = v7_get(us->v7, us->obj, "_txcb", 5);
      if (v7_is_callable(us->v7, txcb)) {
        sj_invoke_cb(us->v7, txcb, us->obj, v7_mk_undefined());
      }
    }
    if (rxb->used > 0) {
      /* Check for JS recv handler. */
      v7_val_t rxcb = esp_sj_uart_get_recv_handler(uart_no);
      if (v7_is_callable(us->v7, rxcb)) {
        if (!us->recv_pending) {
          us->recv_pending = 1;
          sj_invoke_cb(us->v7, rxcb, us->obj, v7_mk_undefined());
          /* Note: Callback has not run yet, it has been scheduled. */
        }
      } else if (us->prompt) {
        uint8_t *cp;
        while (rxb != NULL && cs_rbuf_get(rxb, 1, &cp) == 1) {
          char c = (char) *cp;
          cs_rbuf_consume(rxb, 1);
          sj_prompt_process_char(c);
          /* UART could've been re-initialized by the command from prompt. */
          rxb = esp_uart_rx_buf(uart_no);
        }
      }
    }
  }
  esp_uart_dispatch_bottom(uart_no);
}

static v7_val_t s_uart_proto;

static enum v7_err esp_sj_uart_get_state(struct v7 *v7,
                                         struct esp_sj_uart_state **us) {
  int uart_no = v7_to_number(v7_get(v7, v7_get_this(v7), "_u", 2));
  if (uart_no < 0 || uart_no > 1) {
    return v7_throwf(v7, "Error", "Invalid UART number");
  }
  *us = &sj_us[uart_no];
  return V7_OK;
}

static enum v7_err UART_configure(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;

  struct esp_uart_config *cfg = esp_sj_uart_default_config(us->uart_no);
  v7_val_t cobj = v7_arg(v7, 0);
  if (v7_is_object(cobj)) {
    v7_val_t v;
#define NUM_PROP(p)             \
  v = v7_get(v7, cobj, #p, ~0); \
  if (v7_is_number(v)) cfg->p = v7_to_number(v);
#define BOOL_PROP(p)            \
  v = v7_get(v7, cobj, #p, ~0); \
  if (!v7_is_undefined(v)) cfg->p = v7_to_boolean(v);

    NUM_PROP(baud_rate);

    NUM_PROP(rx_buf_size);
    BOOL_PROP(rx_fc_ena);
    NUM_PROP(rx_fifo_full_thresh);
    NUM_PROP(rx_fifo_fc_thresh);
    NUM_PROP(rx_fifo_alarm);
    NUM_PROP(rx_linger_micros);

    NUM_PROP(tx_buf_size);
    BOOL_PROP(tx_fc_ena);
    NUM_PROP(tx_fifo_empty_thresh);
    NUM_PROP(tx_fifo_full_thresh);

    BOOL_PROP(swap_rxcts_txrts);

    NUM_PROP(status_interval_ms);
  }

  if (esp_uart_init(cfg)) {
    esp_sj_uart_schedule_dispatcher(cfg->uart_no);
    *res = v7_mk_boolean(1);
  } else {
    free(cfg);
    *res = v7_mk_boolean(0);
  }
  return V7_OK;
}

/* Args: callable, disable_rx */
static enum v7_err UART_onRecv(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;
  v7_set(v7, us->obj, "_rxcb", 5, v7_arg(v7, 0));
  esp_uart_set_rx_enabled(us->uart_no, !v7_is_truthy(v7, v7_arg(v7, 1)));
  if (v7_is_callable(v7, v7_arg(v7, 0))) {
    esp_sj_uart_schedule_dispatcher(us->uart_no);
  }
  *res = v7_mk_undefined();
  return V7_OK;
}

static enum v7_err UART_recv(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;

  cs_rbuf_t *rxb = esp_uart_rx_buf(us->uart_no);
  size_t len = MIN((size_t) v7_to_number(v7_arg(v7, 0)), rxb->used);
  uint8_t *data;
  len = cs_rbuf_get(rxb, len, &data);
  *res = v7_mk_string(v7, (const char *) data, len, 1 /* copy */);
  cs_rbuf_consume(rxb, len);
  us->recv_pending = 0;
  /* This is required to unblock interrupts after buffer has been filled.
   * And won't hurt in general. */
  esp_sj_uart_schedule_dispatcher(us->uart_no);
  return V7_OK;
}

static enum v7_err UART_setRXEnabled(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;
  esp_uart_set_rx_enabled(us->uart_no, v7_is_truthy(v7, v7_arg(v7, 0)));
  esp_sj_uart_schedule_dispatcher(us->uart_no);
  *res = v7_mk_undefined();
  return V7_OK;
}
static enum v7_err UART_onTXEmpty(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;
  v7_set(v7, us->obj, "_txcb", 5, v7_arg(v7, 0));
  if (v7_is_callable(v7, v7_arg(v7, 0))) {
    esp_sj_uart_schedule_dispatcher(us->uart_no);
  }
  *res = v7_mk_undefined();
  return V7_OK;
}

static enum v7_err UART_sendAvail(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;
  *res = v7_mk_number(esp_uart_tx_buf(us->uart_no)->avail);
  return V7_OK;
}

static enum v7_err UART_send(struct v7 *v7, v7_val_t *res) {
  struct esp_sj_uart_state *us;
  enum v7_err ret = esp_sj_uart_get_state(v7, &us);
  if (ret != V7_OK) return ret;

  v7_val_t arg0 = v7_arg(v7, 0);
  if (!v7_is_string(arg0)) {
    return v7_throwf(v7, "Error", "String arg required");
  }
  size_t len = 0;
  const char *data = v7_get_string_data(v7, &arg0, &len);
  if (data != NULL && len > 0) {
    cs_rbuf_t *txb = esp_uart_tx_buf(us->uart_no);
    len = MIN(len, txb->avail);
    cs_rbuf_append(txb, (uint8_t *) data, len);
    esp_sj_uart_schedule_dispatcher(us->uart_no);
  }
  *res = v7_mk_number(len);
  return V7_OK;
}

static enum v7_err UART_get(struct v7 *v7, v7_val_t *res) {
  enum v7_err ret = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  int uart_no = v7_to_number(arg0);
  if (v7_is_number(arg0) && (uart_no == 0 || uart_no == 1)) {
    *res = sj_us[uart_no].obj;
  } else {
    ret = v7_throwf(v7, "Error", "Invalid UART number");
  }
  return ret;
}

void esp_sj_uart_init(struct v7 *v7) {
  sj_us[0].v7 = sj_us[1].v7 = v7;
  os_timer_setfn(&sj_us[0].timer, esp_sj_uart_dispatcher, (void *) 0);
  os_timer_setfn(&sj_us[1].timer, esp_sj_uart_dispatcher, (void *) 1);
  s_uart_proto = v7_mk_object(v7);
  v7_set_method(v7, s_uart_proto, "configure", UART_configure);
  v7_set_method(v7, s_uart_proto, "onRecv", UART_onRecv);
  v7_set_method(v7, s_uart_proto, "recv", UART_recv);
  v7_set_method(v7, s_uart_proto, "setRXEnabled", UART_setRXEnabled);
  v7_set_method(v7, s_uart_proto, "onTXEmpty", UART_onTXEmpty);
  v7_set_method(v7, s_uart_proto, "sendAvail", UART_sendAvail);
  v7_set_method(v7, s_uart_proto, "send", UART_send);
  v7_set_method(v7, v7_get_global(v7), "UART", UART_get);

  sj_us[0].uart_no = 0;
  sj_us[0].obj = v7_mk_object(v7);
  v7_set_proto(v7, sj_us[0].obj, s_uart_proto);
  v7_set(v7, sj_us[0].obj, "_u", ~0, v7_mk_number(0));
  v7_own(v7, &sj_us[0].obj);

  sj_us[1].uart_no = 1;
  sj_us[1].obj = v7_mk_object(v7);
  v7_set_proto(v7, sj_us[1].obj, s_uart_proto);
  v7_set(v7, sj_us[1].obj, "_u", ~0, v7_mk_number(1));
  v7_own(v7, &sj_us[1].obj);
}

void esp_sj_uart_set_prompt(int uart_no) {
  sj_us[0].prompt = sj_us[1].prompt = 0;
  if (uart_no >= 0 && uart_no <= 1) {
    sj_us[uart_no].prompt = 1;
    esp_uart_set_rx_enabled(uart_no, 1);
    esp_sj_uart_schedule_dispatcher(uart_no);
  }
}

v7_val_t esp_sj_uart_get_recv_handler(int uart_no) {
  if (uart_no < 0 || uart_no > 1) return v7_mk_undefined();
  return v7_get(sj_us[uart_no].v7, sj_us[uart_no].obj, "_rxcb", 5);
}

size_t esp_sj_uart_write(int uart_no, const void *buf, size_t len) {
  size_t n = 0;
  cs_rbuf_t *txb = esp_uart_tx_buf(uart_no);
  while (n < len) {
    while (txb->avail == 0) {
      esp_uart_dispatch_tx_top(uart_no);
    }
    size_t nw = MIN(len - n, txb->avail);
    cs_rbuf_append(txb, ((uint8_t *) buf) + n, nw);
    n += nw;
  }
  esp_sj_uart_schedule_dispatcher(uart_no);
  return len;
}

struct esp_uart_config *esp_sj_uart_default_config(int uart_no) {
  struct esp_uart_config *cfg = calloc(1, sizeof(*cfg));
  cfg->uart_no = uart_no;
  cfg->dispatch_cb = esp_uart_dispatch_signal_from_isr;
  cfg->baud_rate = 115200;
  cfg->rx_buf_size = cfg->tx_buf_size = 256;
  cfg->rx_fifo_alarm = 10;
  cfg->rx_fifo_full_thresh = 120;
  cfg->rx_linger_micros = 15;
  cfg->tx_fifo_empty_thresh = 10;
  cfg->tx_fifo_full_thresh = 125;
  return cfg;
}
