#if defined(RTOS_SDK) && defined(RTOS_NETWORK_TEST)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "disp_task.h"

#include <sj_mongoose.h>
#include <mongoose.h>

#define NETWORK_SSID <put your SSID here>
#define NETWORK_PWD <put your network pwd here>
#define ADDR_TO_CONNECT <put server address here>

static xTaskHandle disp_task_handle;

void rtos_dispatch_initialize() {
}

void rtos_dispatch_char_handler(int tail) {
}

void rtos_dispatch_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                            v7_val_t args) {
}

enum conn_state {
  DISCONNECTED = 0,
  CONNECTING,
  CONNECTED,
};

struct conn_ctx {
  unsigned int num_sent;
  unsigned int num_received;
  enum conn_state state;
};

#define HWM 1024

char data[4096];

void do_send(struct mg_connection *nc) {
  if (nc->send_mbuf.len > HWM) return;
  mg_send(nc, data, sizeof(data));
}

void conn_handler(struct mg_connection *nc, int ev, void *arg) {
  struct conn_ctx *ctx = (struct conn_ctx *) nc->user_data;
  struct mbuf *io = &nc->recv_mbuf;
  switch (ev) {
    case NS_POLL:
      break;
    case NS_CONNECT: {
      printf("connected\n");
      ctx->state = CONNECTED;
      do_send(nc);
      break;
    }
    case NS_RECV:
      ctx->num_received += io->len;
      mbuf_remove(io, io->len);
      break;
    case NS_SEND: {
      do_send(nc);
      ctx->num_sent += *((int *) arg);
      break;
    }
    case NS_CLOSE: {
      printf("disconnected\n");
      ctx->state = DISCONNECTED;
      break;
    }
  }
}

static int wifi_connected;

static void wifi_change_cb(System_Event_t *evt) {
  printf("wifi event: %u, heap free: %u\n", evt->event_id,
         system_get_free_heap_size());
  if (evt->event_id == EVENT_STAMODE_GOT_IP) wifi_connected = 1;
}

static void wifi_connect() {
  struct station_config conf;
  wifi_station_set_auto_connect(FALSE);
  wifi_set_event_handler_cb(wifi_change_cb);
  strcpy((char *) conf.ssid, NETWORK_SSID);
  strcpy((char *) conf.password, NETWORK_PWD);
  printf("connecting to %s\n", conf.ssid);
  conf.bssid_set = 0;
  wifi_set_opmode_current(STATION_MODE);
  wifi_station_disconnect();
  wifi_station_set_config_current(&conf);
  wifi_station_connect();
}

static void disp_task(void *params) {
  struct mg_connection *conn = NULL;
  struct conn_ctx ctx;
  printf("Starting network test...\n");
  memset(data, 'A', sizeof(data));
  memset(&ctx, 0, sizeof(ctx));
  wifi_connect();
  int i = 0;
  while (1) {
    mongoose_poll(2);
    if (!wifi_connected) continue;
    switch (ctx.state) {
      case DISCONNECTED:
        memset(&ctx, 0, sizeof(ctx));
        conn = mg_connect(&sj_mgr, ADDR_TO_CONNECT, conn_handler);
        if (conn == NULL) {
          vTaskDelay(100);
          printf("reconnecting\n");
          continue;
        }
        printf("conn = %p\n", conn);
        conn->user_data = &ctx;
        ctx.state = CONNECTING;
        break;
      case CONNECTING:
        break;
      case CONNECTED:
        i++;
        if (i % 10000 == 0) {
          printf("sent %u, recv %u heap free: %u uptime %d\n", ctx.num_sent,
                 ctx.num_received, system_get_free_heap_size(),
                 system_get_time() / 1000000);
        }
        break;
    }
  }
}

void rtos_init_dispatcher() {
  xTaskCreate(disp_task, (const signed char *) "disp_task",
              (V7_STACK_SIZE + 1024) / 4, NULL, tskIDLE_PRIORITY + 2,
              &disp_task_handle);
}

#endif /* RTOS_SDK && RTOS_NETWORK_TEST */
