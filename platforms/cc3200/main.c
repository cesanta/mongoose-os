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
#include "rom_map.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"

#include "simplelink.h"
#include "device.h"

#include "oslib/osi.h"

#include "sj_prompt.h"
#include "sj_wifi.h"
#include "v7.h"
#include "config.h"
#include "cc3200_leds.h"
#include "cc3200_sj_hal.h"
#include "cc3200_wifi.h"

struct v7 *v7;
static const char *v7_version = "TODO";

static v7_val_t js_usleep(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t usecsv = v7_array_get(v7, args, 0);
  int msecs;
  if (!v7_is_number(usecsv)) {
    printf("usecs is not a double\n\r");
    return v7_create_undefined();
  }
  msecs = v7_to_number(usecsv) / 1000;
  osi_Sleep(msecs);
  return v7_create_undefined();
}

void init_v7(void *stack_base) {
  struct v7_create_opts opts;

  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
  opts.c_stack_base = stack_base;

  v7 = v7_create_opt(opts);

  v7_set(v7, v7_get_global_object(v7), "version", 7, 0,
         v7_create_string(v7, v7_version, strlen(v7_version), 1));
  v7_set_method(v7, v7_get_global_object(v7), "usleep", js_usleep);
  v7_gc(v7, 1);
}

static void blinkenlights_task(void *arg) {
  while (1) {
    cc3200_leds(GREEN, TOGGLE);
    osi_Sleep(500);
  }
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
  fprintf(stderr, "stack overflow! %s (%p)\n", tn, th);
}

OsiMsgQ_t s_prompt_input_q;

static void uart_int() {
  int c = UARTCharGet(CONSOLE_UART);
  struct prompt_event pe = {.type = PROMPT_CHAR_EVENT, .data = (void *) c};
  osi_MsgQWrite(&s_prompt_input_q, &pe, OSI_NO_WAIT);
  MAP_UARTIntClear(CONSOLE_UART, UART_INT_RX);
}

void sj_prompt_init_hal(struct v7 *v7) {
  (void) v7;
}

static void prompt_task(void *arg) {
  int dummy;

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
  printf("\n\nSmart.JS for CC3200\n");

  osi_MsgQCreate(&s_prompt_input_q, "prompt input", sizeof(struct prompt_event),
                 32 /* len */);
  osi_InterruptRegister(CONSOLE_UART_INT, uart_int, INT_PRIORITY_LVL_1);
  MAP_UARTIntEnable(CONSOLE_UART, UART_INT_RX);
  sl_Start(NULL, NULL, NULL);
  init_v7(&dummy);
  init_wifi(v7);
  sj_prompt_init(v7);
  while (1) {
    struct prompt_event pe;
    osi_MsgQRead(&s_prompt_input_q, &pe, OSI_WAIT_FOREVER);
    switch (pe.type) {
      case PROMPT_CHAR_EVENT: {
        sj_prompt_process_char((char) ((int) pe.data));
        break;
      }
      case V7_EXEC_EVENT: {
        struct v7_exec_event_data *ped = (struct v7_exec_event_data *) pe.data;
        v7_val_t res;
        if (v7_exec_with(v7, &res, ped->code, ped->this_obj) != V7_OK) {
          v7_fprintln(stderr, v7, res);
        }
        v7_disown(v7, &ped->this_obj);
        free(ped->code);
        free(ped);
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
  VStartSimpleLinkSpawnTask(8);
  osi_TaskCreate(prompt_task, "prompt", V7_STACK_SIZE + 256, NULL, 2, NULL);
  osi_TaskCreate(blinkenlights_task, "blink", 256, NULL, 1, NULL);
  osi_start();

  return 0;
}
