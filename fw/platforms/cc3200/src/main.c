/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __TI_COMPILER_VERSION__
#include <unistd.h>
#endif

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

#include "common/platform.h"

#include "simplelink.h"
#include "device.h"

#include "oslib/osi.h"

#include "fw/src/device_config.h"
#include "fw/src/sj_app.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_http.h"
#include "fw/src/sj_gpio.h"
#include "fw/src/sj_gpio_js.h"
#include "fw/src/sj_i2c_js.h"
#include "fw/src/sj_prompt.h"
#include "fw/src/sj_timers.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_updater_post.h"
#include "fw/src/sj_v7_ext.h"
#include "fw/src/sj_wifi_js.h"
#include "fw/src/sj_wifi.h"
#include "v7/v7.h"

#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_fs.h"
#include "fw/platforms/cc3200/src/cc3200_sj_hal.h"
#include "fw/platforms/cc3200/src/cc3200_updater.h"

extern const char *build_id;

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
  exit(123);
}

void vApplicationIdleHook() {
  /* Ho-hum. Twiddling our thumbs. */
}

void vApplicationStackOverflowHook(OsiTaskHandle *th, signed char *tn) {
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *e) {
}

OsiMsgQ_t s_v7_q;

#ifndef CS_DISABLE_JS
static void uart_int() {
  struct sj_event e = {.type = PROMPT_CHAR_EVENT, .data = NULL};
  MAP_UARTIntClear(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  MAP_UARTIntDisable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  osi_MsgQWrite(&s_v7_q, &e, OSI_NO_WAIT);
}
#endif

void sj_prompt_init_hal(struct v7 *v7) {
  (void) v7;
}

static f_gpio_intr_handler_t s_gpio_js_handler;
static void *s_gpio_js_handler_arg;

void sj_gpio_intr_init(f_gpio_intr_handler_t cb, void *arg) {
  s_gpio_js_handler = cb;
  s_gpio_js_handler_arg = arg;
}

int start_nwp() {
  int r = sl_Start(NULL, NULL, NULL);
  if (r < 0) return r;
  SlVersionFull ver;
  unsigned char opt = SL_DEVICE_GENERAL_VERSION;
  unsigned char len = sizeof(ver);

  memset(&ver, 0, sizeof(ver));
  sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &opt, &len,
            (unsigned char *) (&ver));
  LOG(LL_INFO, ("NWP v%d.%d.%d.%d started, host driver v%d.%d.%d.%d",
                ver.NwpVersion[0], ver.NwpVersion[1], ver.NwpVersion[2],
                ver.NwpVersion[3], SL_MAJOR_VERSION_NUM, SL_MINOR_VERSION_NUM,
                SL_VERSION_NUM, SL_SUB_VERSION_NUM));
  return 0;
}

static void main_task(void *arg) {
  struct v7 *v7 = s_v7;

  cs_log_set_level(LL_INFO);
  LOG(LL_INFO, ("Mongoose IoT Firmware %s", build_id));
  LOG(LL_INFO,
      ("RAM: %d total, %d free", sj_get_heap_size(), sj_get_free_heap_size()));

  int r = start_nwp();
  if (r < 0) {
    LOG(LL_ERROR, ("Failed to start NWP: %d", r));
    return;
  }

  osi_MsgQCreate(&s_v7_q, "V7", sizeof(struct sj_event), 32 /* len */);

  int boot_cfg_idx = get_active_boot_cfg_idx();
  if (boot_cfg_idx < 0) return;
  struct boot_cfg boot_cfg;
  if (read_boot_cfg(boot_cfg_idx, &boot_cfg) < 0) return;

  LOG(LL_INFO, ("Boot cfg %d: 0x%llx, 0x%u, %s @ 0x%08x, %s", boot_cfg_idx,
                boot_cfg.seq, boot_cfg.flags, boot_cfg.app_image_file,
                boot_cfg.app_load_addr, boot_cfg.fs_container_prefix));

  uint64_t saved_seq = 0;
  if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    /* Tombstone the current config. If anything goes wrong between now and
     * commit, next boot will use the old one. */
    saved_seq = boot_cfg.seq;
    boot_cfg.seq = BOOT_CFG_TOMBSTONE_SEQ;
    write_boot_cfg(&boot_cfg, boot_cfg_idx);
  }

  r = init_fs(boot_cfg.fs_container_prefix);
  if (r < 0) {
    LOG(LL_ERROR, ("FS init error: %d", r));
    if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
      revert_update(boot_cfg_idx, &boot_cfg);
    }
    return;
  }

  if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    LOG(LL_INFO, ("Applying update"));
    if (apply_update(boot_cfg_idx, &boot_cfg) < 0) {
      revert_update(boot_cfg_idx, &boot_cfg);
    }
  }

  mongoose_init();

#ifndef CS_DISABLE_JS
  v7 = s_v7 = init_v7(&v7);

  /* Disable GC during JS API initialization. */
  v7_set_gc_enabled(v7, 0);
  sj_gpio_api_setup(v7);
  sj_i2c_api_setup(v7);
  sj_wifi_api_setup(v7);
  sj_timers_api_setup(v7);
#endif

  sj_v7_ext_api_setup(v7);
  sj_init_sys(v7);
  sj_wifi_init(v7);

  sj_http_api_setup(v7);

  /* Common config infrastructure. Mongoose & v7 must be initialized. */
  init_device(v7);

#ifndef CS_DISABLE_JS
  /* SJS initialized, enable GC back, and trigger it */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

  v7_val_t res;
  if (v7_exec_file(v7, "sys_init.js", &res) != V7_OK) {
    fprintf(stderr, "Error: ");
    v7_fprint(stderr, v7, res);
  }
#endif

  sj_updater_post_init(v7);

  LOG(LL_INFO, ("Sys init done, RAM: %d free", sj_get_free_heap_size()));

  if (!sj_app_init(v7)) {
    LOG(LL_ERROR, ("App init failed"));
    abort();
  }
  LOG(LL_INFO, ("App init done"));

  if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    boot_cfg.seq = saved_seq;
    commit_update(boot_cfg_idx, &boot_cfg);
  }

#ifndef CS_DISABLE_JS
  osi_InterruptRegister(CONSOLE_UART_INT, uart_int, INT_PRIORITY_LVL_1);
  MAP_UARTIntEnable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  sj_prompt_init(v7);
#endif

  while (1) {
    struct sj_event e;
    mongoose_poll(0);
    if (osi_MsgQRead(&s_v7_q, &e, V7_POLL_LENGTH_MS) != OSI_OK) continue;
    switch (e.type) {
#ifndef CS_DISABLE_JS
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
#endif
      case GPIO_INT_EVENT: {
        int pin = ((intptr_t) e.data) >> 1;
        enum gpio_level val =
            ((intptr_t) e.data) & 1 ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
        if (s_gpio_js_handler != NULL)
          s_gpio_js_handler(pin, val, s_gpio_js_handler_arg);
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
  osi_TaskCreate(main_task, (const signed char *) "main", V7_STACK_SIZE + 256,
                 NULL, 3, NULL);
  osi_start();

  return 0;
}

/* FreeRTOS assert() hook. */
void vAssertCalled(const char *pcFile, unsigned long ulLine) {
  // Handle Assert here
  while (1) {
  }
}

int sj_app_init(struct v7 *v7) __attribute__((weak));
int sj_app_init(struct v7 *v7) {
  (void) v7;
  return 1;
}
