#include "mbed.h"
#include "pinmap.h"
#include "PeripheralPins.h"
#include "stm32_uart.h"
#include "common/cs_dbg.h"
#include "fw/src/mgos_uart.h"
#include <map>

int s_stdout_uart_no;
int s_stderr_uart_no;

class MGOSSerial : public RawSerial {
 public:
  MGOSSerial(int uart_no, PinName pin_tx, PinName pin_rx, int baud_rate)
      : RawSerial(pin_tx, pin_rx, baud_rate), uart_no_(uart_no) {
    cs_rbuf_init(&rx_buf_int_, 1024);
  };

  void rx_irq_handler() {
    /*
     * 1. We should not disable interrupts, it is done by caller of this
     * function (i.e. mbed)
     * 2. If interruptions is turned ON, serial is always empty
     * outside this handler. Looks like mbed do this (there are a tons of code)
     * in "real" interrupt handler
     * TODO(alashkin): check this out
     */
    while (readable() && rx_buf_int_.avail > 0) {
      cs_rbuf_append_one(&rx_buf_int_, getc());
    }
    mgos_uart_schedule_dispatcher(uart_no_);
  }

  void enable_rx_irq() {
    attach(Callback<void()>(this, &MGOSSerial::rx_irq_handler),
           SerialBase::RxIrq);
  }

  void disable_rx_irq() {
    attach(0, SerialBase::RxIrq);
  }

  void get_rx_data(cs_rbuf_t *buf) {
    if (buf->avail == 0 || rx_buf_int_.used == 0) {
      return;
    }
    uint8_t *cp;
    int len = buf->avail > rx_buf_int_.used ? rx_buf_int_.used : buf->avail;
    cs_rbuf_get(&rx_buf_int_, len, &cp);
    cs_rbuf_append(buf, cp, len);
    cs_rbuf_consume(&rx_buf_int_, len);
  }

 private:
  cs_rbuf_t rx_buf_int_;
  int uart_no_;
};

int mgos_stm32_get_stdout_uart() {
  return s_stdout_uart_no;
}

int mgos_stm32_get_stderr_uart() {
  return s_stderr_uart_no;
}

void mgos_uart_dev_set_defaults(struct mgos_uart_config *cfg) {
  /* Do nothing here */
  (void) cfg;
}

void mgos_uart_dev_dispatch_rx_top(struct mgos_uart_state *us) {
  MGOSSerial *serial = (MGOSSerial *) us->dev_data;
  if (serial == NULL) {
    return;
  }

  serial->get_rx_data(&us->rx_buf);
}

void mgos_uart_dev_dispatch_tx_top(struct mgos_uart_state *us) {
  MGOSSerial *serial = (MGOSSerial *) us->dev_data;

  if (serial == NULL) {
    return;
  }

  while (us->tx_buf.used != 0 && serial->writeable() != 0) {
    uint8_t *cp;
    if (cs_rbuf_get(&us->tx_buf, 1, &cp) == 1) {
      serial->putc(*cp);
      cs_rbuf_consume(&us->tx_buf, 1);
    }
  }
}

void mgos_uart_dev_dispatch_bottom(struct mgos_uart_state *us) {
  /*
   * Do nothing here, coz enabling/disabling interruptions
   * conflicts with mbed logic
   * TODO(alashkin): have mbed
   */
}

static void init_uart(PinName pin_tx, PinName pin_rx,
                      struct mgos_uart_state *us) {
  us->dev_data =
      new MGOSSerial(us->uart_no, pin_tx, pin_rx, us->cfg->baud_rate);
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

bool mgos_uart_dev_init(struct mgos_uart_state *us) {
  /*
    * DISCOVERY boards have a different number of UARTS, at this moment
    * we support 3 (like stdout, strerr + user), because it is unclear how
    * to put platform specific number uarts parameters to configuration
    * TODO(alex): find a beautiful way
    */
  if (us->dev_data != 0) {
    /* already initialized, should be deinited before this call */
    return true;
  }
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

void mgos_uart_dev_deinit(struct mgos_uart_state *us) {
  MGOSSerial *serial = (MGOSSerial *) us->dev_data;
  if (serial == NULL) {
    return;
  }

  delete serial;
  us->dev_data = NULL;
}

void mgos_uart_dev_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  MGOSSerial *serial = (MGOSSerial *) us->dev_data;
  if (serial == NULL) {
    return;
  }

  if (enabled) {
    serial->enable_rx_irq();
  } else {
    serial->disable_rx_irq();
  }
}

enum mgos_init_result mgos_set_stdout_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stdout_uart_no = uart_no;
  }

  return r;
}

enum mgos_init_result mgos_set_stderr_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stderr_uart_no = uart_no;
  }
  return r;
}
