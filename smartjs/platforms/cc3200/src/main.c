/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Driverlib includes */
#include "hw_types.h"

#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

#include "simplelink.h"
#include "device.h"

#include "oslib/osi.h"

#include "smartjs/src/device_config.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_http.h"
#include "smartjs/src/sj_gpio.h"
#include "smartjs/src/sj_gpio_js.h"
#include "smartjs/src/sj_i2c_js.h"
#include "smartjs/src/sj_prompt.h"
#include "smartjs/src/sj_timers.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_wifi_js.h"
#include "smartjs/src/sj_wifi.h"
#include "v7/v7.h"

#include "config.h"
#include "cc3200_fs.h"
#include "cc3200_sj_hal.h"

const char *build_id;

struct v7 *s_v7;

struct v7 *init_v7(void *stack_base) {
  struct v7_create_opts opts;

  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
  opts.c_stack_base = stack_base;

  return v7_create_opt(opts);
}

/* These are FreeRTOS hooks for various life situations. */
void vApplicationMallocFailedHook() {
  fprintf(stderr, "malloc failed\n");
  _exit(123);
}

void vApplicationIdleHook() {
  /* Ho-hum. Twiddling our thumbs. */
}

void vApplicationStackOverflowHook(OsiTaskHandle *th, signed char *tn) {
}

OsiMsgQ_t s_v7_q;

static void uart_int() {
  struct sj_event e = {.type = PROMPT_CHAR_EVENT, .data = NULL};
  MAP_UARTIntClear(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  MAP_UARTIntDisable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  osi_MsgQWrite(&s_v7_q, &e, OSI_NO_WAIT);
}

void sj_prompt_init_hal(struct v7 *v7) {
  (void) v7;
}

static f_gpio_intr_handler_t s_gpio_js_handler;

void sj_gpio_intr_init(f_gpio_intr_handler_t cb) {
  s_gpio_js_handler = cb;
}

static void v7_task(void *arg) {
  struct v7 *v7 = s_v7;
  printf("\n\nSmart.js %s\n", build_id);

  osi_MsgQCreate(&s_v7_q, "V7", sizeof(struct sj_event), 32 /* len */);
  osi_InterruptRegister(CONSOLE_UART_INT, uart_int, INT_PRIORITY_LVL_1);
  MAP_UARTIntEnable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  sl_Start(NULL, NULL, NULL);

  mongoose_init();
  v7 = s_v7 = init_v7(&v7);
  sj_timers_api_setup(v7);
  sj_v7_ext_api_setup(v7);
  sj_init_sys(v7);
  sj_wifi_api_setup(v7);
  sj_wifi_init(v7);
  if (init_fs(v7) != 0) {
    fprintf(stderr, "FS initialization failed.\n");
  }

  sj_gpio_init(v7);
  sj_gpio_api_setup(v7);
  sj_http_api_setup(v7);
  sj_i2c_api_setup(v7);

  /* Common config infrastructure. Mongoose & v7 must be initialized. */
  init_device(v7);

  v7_val_t res;
  if (v7_exec_file(v7, "sys_init.js", &res) != V7_OK) {
    fprintf(stderr, "Error: ");
    v7_fprint(stderr, v7, res);
  }

  fprintf(stderr, "RAM: %d total, %d free\n", sj_get_heap_size(),
          sj_get_free_heap_size());

  sj_prompt_init(v7);

  while (1) {
    struct sj_event e;
    mongoose_poll(0);
    if (osi_MsgQRead(&s_v7_q, &e, V7_POLL_LENGTH_MS) != OSI_OK) continue;
    switch (e.type) {
      case PROMPT_CHAR_EVENT: {
        long c;
        while ((c = UARTCharGetNonBlocking(CONSOLE_UART)) >= 0) {
          sj_prompt_process_char(c);
        }
        MAP_UARTIntEnable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
        break;
      }
      case V7_INVOKE_EVENT: {
        struct v7_invoke_event_data *ied =
            (struct v7_invoke_event_data *) e.data;
        _sj_invoke_cb(v7, ied->func, ied->this_obj, ied->args);
        v7_disown(v7, &ied->args);
        v7_disown(v7, &ied->this_obj);
        v7_disown(v7, &ied->func);
        free(ied);
        break;
      }
      case GPIO_INT_EVENT: {
        int pin = ((intptr_t) e.data) >> 1;
        int val = ((intptr_t) e.data) & 1;
        if (s_gpio_js_handler != NULL) s_gpio_js_handler(pin, val);
        break;
      }
      case MG_POLL_EVENT: {
        /* Nothing to do, we poll on every iteration anyway. */
        break;
      }
    }
  }
}

/* Int vector table, defined in startup_gcc.c */
extern void (*const g_pfnVectors[])(void);

void device_reboot(void) {
  sj_system_restart(0);
}

int main() {
  MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
  MAP_IntEnable(FAULT_SYSTICK);
  MAP_IntMasterEnable();
  PRCMCC3200MCUInit();

  /* Console UART init. */
  MAP_PRCMPeripheralClkEnable(CONSOLE_UART_PERIPH, PRCM_RUN_MODE_CLK);
  MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* PIN_55 -> UART0_TX */
  MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* PIN_57 -> UART0_RX */
  MAP_UARTConfigSetExpClk(
      CONSOLE_UART, MAP_PRCMPeripheralClockGet(CONSOLE_UART_PERIPH),
      CONSOLE_BAUD_RATE,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  MAP_UARTFIFOLevelSet(CONSOLE_UART, UART_FIFO_TX1_8, UART_FIFO_RX4_8);
  MAP_UARTFIFOEnable(CONSOLE_UART);

  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);
  cs_log_set_level(LL_INFO);

  VStartSimpleLinkSpawnTask(8);
  osi_TaskCreate(v7_task, (const signed char *) "v7", V7_STACK_SIZE + 256, NULL,
                 3, NULL);
  osi_start();

  return 0;
}
