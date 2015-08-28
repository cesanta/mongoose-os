#ifdef RTOS_SDK

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static xTaskHandle disp_task_handle;
static xQueueHandle main_queue_handle;

/* Add new events to this enum */
enum rtos_events {
  RTE_INIT /* no params */,
  RTE_UART_NEWCHAR /* `uart_rx_params` in `rtos_event` are params */,
};

struct rtos_event {
  union {
    struct {
      int tail;
    } uart_rx_params;
    /* Add parameters for new events to this union */
  } params;

  enum rtos_events event_id;
};

/* Add function to call declaration here */
void start_cmd(void *dummy);
void process_rx_buf(int tail);

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

static void disp_task(void *params) {
  struct rtos_event ev;

  while (1) {
    if (xQueueReceive(main_queue_handle, (void *) &ev,
                      (portTickType) portMAX_DELAY)) {
      switch (ev.event_id) {
        case RTE_INIT:
          start_cmd(0);
          break;
        case RTE_UART_NEWCHAR:
          process_rx_buf(ev.params.uart_rx_params.tail);
          break;
        default:
          printf("Unknown event_id: %d\n", ev.event_id);
      }
    }
  }
}

void rtos_init_dispatcher() {
  main_queue_handle = xQueueCreate(32, sizeof(struct rtos_event));
  xTaskCreate(disp_task, (const signed char *) "disp_task",
              (V7_STACK_SIZE + 256) / 4, NULL, tskIDLE_PRIORITY + 2,
              &disp_task_handle);
}

#endif /* RTOS_SDK */
