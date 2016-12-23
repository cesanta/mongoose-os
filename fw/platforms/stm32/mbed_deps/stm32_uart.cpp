#include "mbed.h"
#include "pinmap.h"
#include "PeripheralPins.h"
#include "stm32_uart.h"
#include "common/cs_dbg.h"
#include "fw/src/miot_uart.h"
#include <map>

typedef std::map<int, serial_t> serials_map_t;
static serials_map_t s_serials_map;
int s_stdout_uart_no;
int s_stderr_uart_no;

static serial_t *get_serial_by_uart_no(int uart_no) {
  serials_map_t::iterator i = s_serials_map.find(uart_no);
  if (i == s_serials_map.end()) {
    return NULL;
  }

  return &i->second;
}

int miot_stm32_get_stdout_uart() {
  return s_stdout_uart_no;
}

int miot_stm32_get_stderr_uart() {
  return s_stderr_uart_no;
}

static void irq_handler(uint32_t id, SerialIrq event) {
  serial_t *serial = get_serial_by_uart_no(id);
  if (serial == NULL) {
    /* Cannot print error from intr handler */
    return;
  }
  serial_irq_set(serial, event, false);
  miot_uart_schedule_dispatcher(id);
}

void miot_uart_dev_set_defaults(struct miot_uart_config *cfg) {
  /* Do nothing here */
  (void) cfg;
}

void miot_uart_dev_dispatch_rx_top(struct miot_uart_state *us) {
  serial_t *serial = get_serial_by_uart_no(us->uart_no);
  if (serial == NULL) {
    LOG(LL_ERROR, ("UART %d is not initialized", us->uart_no));
    return;
  }

  while (us->rx_buf.avail > 0 && serial_readable(serial) != 0) {
    cs_rbuf_append_one(&us->rx_buf, serial_getc(serial));
  }
}

void miot_uart_dev_dispatch_tx_top(struct miot_uart_state *us) {
  serial_t *serial = get_serial_by_uart_no(us->uart_no);
  if (serial == NULL) {
    LOG(LL_ERROR, ("UART %d is not initialized", us->uart_no));
    return;
  }

  while (us->tx_buf.used != 0 && serial_writable(serial) != 0) {
    uint8_t *cp;
    if (cs_rbuf_get(&us->tx_buf, 1, &cp) == 1) {
      serial_putc(serial, *cp);
      cs_rbuf_consume(&us->tx_buf, 1);
    }
  }
}

void miot_uart_dev_dispatch_bottom(struct miot_uart_state *us) {
  serial_t *serial = get_serial_by_uart_no(us->uart_no);
  if (serial == NULL) {
    LOG(LL_ERROR, ("UART %d is not initialized", us->uart_no));
    return;
  }

  if (us->rx_enabled && us->rx_buf.avail > 0) {
    serial_irq_set(serial, RxIrq, true);
  }

  if (us->tx_buf.used > 0) {
    serial_irq_set(serial, TxIrq, true);
  }
}

static void init_uart(PinName pin_tx, PinName pin_rx,
                      struct miot_uart_state *us) {
  serial_t serial;

  serial_init(&serial, pin_tx, pin_rx);
  serial_baud(&serial, us->cfg->baud_rate);

  s_serials_map.insert(std::make_pair(us->uart_no, serial));
}

static PinName get_uart_pin(UARTName uart_name, const PinMap *map) {
  while (map->pin != NC) {
    if (map->peripheral == uart_name) return map->pin;
    map++;
  }
  return NC;
}

static int get_uart_pins(UARTName uart_name, PinName *pin_tx, PinName *pin_rx) {
  *pin_tx = get_uart_pin(uart_name, PinMap_UART_TX);
  *pin_rx = get_uart_pin(uart_name, PinMap_UART_RX);

  return (*pin_tx != NC && *pin_rx != NC);
}

bool miot_uart_dev_init(struct miot_uart_state *us) {
  /*
    * DISCOVERY boards have a different number of UARTS, at this moment
    * we support 3 (like stdout, strerr + user), because it is unclear how
    * to put platform specific number uarts parameters to configuration
    * TODO(alex): find a beautiful way
    */
  if (us->uart_no > 2) {
    LOG(LL_ERROR, ("Invalid uart no: %d", us->uart_no));
    return false;
  }

  PinName pin_tx, pin_rx;

  if (us->uart_no == 0) {
    /*
     * For uart 0 we use default UART pins. On DISCOVERY boards
     * they are usually also mirrorred to USB ST-link connection
     */
    pin_tx = SERIAL_TX;
    pin_rx = SERIAL_RX;
  } else {
    if (!get_uart_pins(us->uart_no == 1 ? UART_2 : UART_3, &pin_tx, &pin_rx)) {
      return false;
    }
  }

  init_uart(pin_tx, pin_rx, us);

  return true;
}

void miot_uart_dev_deinit(struct miot_uart_state *us) {
  serials_map_t::iterator i = s_serials_map.find(us->uart_no);

  if (i == s_serials_map.end()) {
    LOG(LL_WARN, ("UART %d is not initialized", us->uart_no));
    return;
  }

  serial_free(&i->second);
  s_serials_map.erase(i);
}

void miot_uart_dev_set_rx_enabled(struct miot_uart_state *us, bool enabled) {
  serial_t *serial = get_serial_by_uart_no(us->uart_no);
  if (serial == NULL) {
    LOG(LL_ERROR, ("UART %d is not initialized", us->uart_no));
    return;
  }

  if (enabled) {
    serial_irq_handler(serial, irq_handler, us->uart_no);
  }

  serial_irq_set(serial, RxIrq, enabled);
}

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stdout_uart_no = uart_no;
  }

  return r;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stderr_uart_no = uart_no;
  }
  return r;
}
