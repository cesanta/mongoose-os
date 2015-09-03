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

#include "sj_fossa.h"
#include "sj_prompt.h"
#include "sj_v7_ext.h"
#include "sj_wifi.h"
#include "v7.h"
#include "config.h"
#include "cc3200_fs.h"
#include "cc3200_leds.h"
#include "cc3200_sj_hal.h"
#include "cc3200_wifi.h"

struct v7 *v7;
const char *sj_version = "TODO";

void init_v7(void *stack_base) {
  struct v7_create_opts opts;

  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
  opts.c_stack_base = stack_base;

  v7 = v7_create_opt(opts);
}

static void blinkenlights_task(void *arg) {
  while (1) {
    cc3200_leds(GREEN, TOGGLE);
    osi_Sleep(500);
  }
}

/* These are FreeRTOS hooks for various life situations. */
void vApplicationMallocFailedHook() {
  cc3200_leds(RED, ON);
  fprintf(stderr, "malloc failed\n");
  _exit(123);
}

void vApplicationIdleHook() {
  /* Ho-hum. Twiddling our thumbs. */
}

void vApplicationStackOverflowHook(OsiTaskHandle *th, signed char *tn) {
  cc3200_leds(RED, ON);
}

OsiMsgQ_t s_v7_q;

static void uart_int() {
  int c = UARTCharGet(CONSOLE_UART);
  struct prompt_event pe = {.type = PROMPT_CHAR_EVENT, .data = (void *) c};
  osi_MsgQWrite(&s_v7_q, &pe, OSI_NO_WAIT);
  MAP_UARTIntClear(CONSOLE_UART, UART_INT_RX);
}

void sj_prompt_init_hal(struct v7 *v7) {
  (void) v7;
}

static void fossa_poll_task(void *arg) {
  fossa_init();
  while (1) {
    if (!fossa_poll()) osi_Sleep(2);
  }
}

static void v7_task(void *arg) {
  char dummy;
  printf("\n\nSmart.JS for CC3200\n");

  osi_MsgQCreate(&s_v7_q, "V7", sizeof(struct prompt_event), 32 /* len */);
  osi_InterruptRegister(CONSOLE_UART_INT, uart_int, INT_PRIORITY_LVL_1);
  MAP_UARTIntEnable(CONSOLE_UART, UART_INT_RX);
  sl_Start(NULL, NULL, NULL);
  init_v7(&dummy);
  sj_init_v7_ext(v7);
  init_wifi(v7);
  if (init_fs(v7) != 0) {
    fprintf(stderr, "FS initialization failed.\n");
  }
  sj_init_simple_http_client(v7);
  osi_TaskCreate(fossa_poll_task, (const signed char *) "fossa", 7 * 1024, NULL,
                 2, NULL);
  sj_prompt_init(v7);
  while (1) {
    struct prompt_event pe;
    osi_MsgQRead(&s_v7_q, &pe, OSI_WAIT_FOREVER);
    switch (pe.type) {
      case PROMPT_CHAR_EVENT: {
        sj_prompt_process_char((char) ((int) pe.data));
        break;
      }
      case V7_INVOKE_EVENT: {
        struct v7_invoke_event_data *ied =
            (struct v7_invoke_event_data *) pe.data;
        _sj_invoke_cb(v7, ied->func, ied->this_obj, ied->args);
        v7_disown(v7, &ied->args);
        v7_disown(v7, &ied->this_obj);
        v7_disown(v7, &ied->func);
        free(ied);
        break;
      }
    }
  }
}

/* Int vector table, defined in startup_gcc.c */
extern void (*const g_pfnVectors[])(void);

int main() {
  MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
  MAP_IntEnable(FAULT_SYSTICK);
  MAP_IntMasterEnable();
  PRCMCC3200MCUInit();

  cc3200_leds_init();

  /* Console UART init. */
  MAP_PRCMPeripheralClkEnable(CONSOLE_UART_PERIPH, PRCM_RUN_MODE_CLK);
  MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* PIN_55 -> UART0_TX */
  MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* PIN_57 -> UART0_RX */
  MAP_UARTConfigSetExpClk(
      CONSOLE_UART, MAP_PRCMPeripheralClockGet(CONSOLE_UART_PERIPH),
      CONSOLE_BAUD_RATE,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  MAP_UARTFIFODisable(CONSOLE_UART);

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  VStartSimpleLinkSpawnTask(8);
  osi_TaskCreate(v7_task, (const signed char *) "v7", V7_STACK_SIZE + 256, NULL,
                 3, NULL);
  osi_TaskCreate(blinkenlights_task, (const signed char *) "blink", 256, NULL,
                 9, NULL);
  osi_start();

  return 0;
}
