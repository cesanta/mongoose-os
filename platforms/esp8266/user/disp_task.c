#if defined(RTOS_SDK) && !defined(RTOS_NETWORK_TEST)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "disp_task.h"

#include <sj_mongoose.h>

static xTaskHandle disp_task_handle;
static xQueueHandle main_queue_handle;

/* Add new events to this enum */
enum rtos_events {
  RTE_INIT /* no params */,
  RTE_UART_NEWCHAR /* `uart_rx_params` in `rtos_event` are params */,
  RTE_CALLBACK /* `*callback_params` in `rtos_event` */,
  RTE_GPIO_INTR_CALLBACK /* `gpio_intr_callback_params` in `rtos_event` */
};

struct rtos_event {
  union {
    struct {
      int tail;
    } uart_rx_params;
    struct {
      struct v7 *v7;
      v7_val_t func;
      v7_val_t this_obj;
      v7_val_t args;
    } * callback_params;
    struct {
      f_gpio_intr_handler_t cb;
      int p1;
      int p2;
    } gpio_intr_callback_params;
    /* Add parameters for new events to this union */
  } params;

  enum rtos_events event_id;
};

/* Add function to call declaration here */
void start_cmd(void *dummy);
void process_rx_buf(int tail);
int mongoose_has_connections();
void _sj_invoke_cb(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                   v7_val_t args);

/* Add helper for new event here */
void rtos_dispatch_initialize() {
  struct rtos_event ev;
  ev.event_id = RTE_INIT;
  xQueueSend(main_queue_handle, &ev, portMAX_DELAY);
}

void rtos_dispatch_char_handler(int tail) {
  struct rtos_event ev;
  ev.event_id = RTE_UART_NEWCHAR;
  ev.params.uart_rx_params.tail = tail;
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  xQueueSendFromISR(main_queue_handle, (void *) &ev, &xHigherPriorityTaskWoken);
  portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void rtos_dispatch_gpio_callback(f_gpio_intr_handler_t cb, int p1, int p2) {
  struct rtos_event ev;

  ev.event_id = RTE_GPIO_INTR_CALLBACK;
  ev.params.gpio_intr_callback_params.cb = cb;
  ev.params.gpio_intr_callback_params.p1 = p1;
  ev.params.gpio_intr_callback_params.p2 = p2;

  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  xQueueSendFromISR(main_queue_handle, (void *) &ev, &xHigherPriorityTaskWoken);
  portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void rtos_dispatch_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                            v7_val_t args) {
  struct rtos_event ev;

  ev.event_id = RTE_CALLBACK;
  ev.params.callback_params = malloc(sizeof(*ev.params.callback_params));
  ev.params.callback_params->v7 = v7;

  ev.params.callback_params->func = func;
  v7_own(v7, &ev.params.callback_params->func);

  ev.params.callback_params->this_obj = this_obj;
  v7_own(v7, &ev.params.callback_params->this_obj);

  ev.params.callback_params->args = args;
  v7_own(v7, &ev.params.callback_params->args);

  xQueueSend(main_queue_handle, &ev, portMAX_DELAY);
}

static void disp_task(void *params) {
  struct rtos_event ev;

  while (1) {
    if (xQueueReceive(main_queue_handle, (void *) &ev,
                      500 / portTICK_RATE_MS)) {
      switch (ev.event_id) {
        case RTE_INIT:
          start_cmd(0);
          break;
        case RTE_UART_NEWCHAR:
          process_rx_buf(ev.params.uart_rx_params.tail);
          break;
        case RTE_CALLBACK:
          _sj_invoke_cb(ev.params.callback_params->v7,
                        ev.params.callback_params->func,
                        ev.params.callback_params->this_obj,
                        ev.params.callback_params->args);
          v7_disown(ev.params.callback_params->v7,
                    &ev.params.callback_params->func);
          v7_disown(ev.params.callback_params->v7,
                    &ev.params.callback_params->this_obj);
          v7_disown(ev.params.callback_params->v7,
                    &ev.params.callback_params->args);
          free(ev.params.callback_params);
          break;
        case RTE_GPIO_INTR_CALLBACK:
          ev.params.gpio_intr_callback_params.cb(
              ev.params.gpio_intr_callback_params.p1,
              ev.params.gpio_intr_callback_params.p2);
          break;
        default:
          printf("Unknown event_id: %d\n", ev.event_id);
          break;
      }
    } else {
      /* Put periodic event handlers here */
      mongoose_poll(2);
    }
  }
}

void rtos_init_dispatcher() {
  main_queue_handle = xQueueCreate(32, sizeof(struct rtos_event));
  xTaskCreate(disp_task, (const signed char *) "disp_task",
              (V7_STACK_SIZE + 1024) / 4, NULL, tskIDLE_PRIORITY + 2,
              &disp_task_handle);
}

#endif /* RTOS_SDK && !RTOS_NETWORK_TEST */
