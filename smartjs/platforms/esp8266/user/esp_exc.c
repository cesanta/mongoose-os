/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "esp_exc.h"

#include <stdio.h>
#include <string.h>
#include <ets_sys.h>
#include <xtensa/corebits.h>
#include <stdint.h>

#include "common/base64.h"
#include "common/cs_dbg.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include "esp_coredump.h"
#include "esp_flash_bytes.h"
#include "esp_fs.h"
#include "esp_gdb.h"
#include "esp_hw.h"
#include "esp_uart.h"
#include "v7_esp.h"

#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <mem.h>

/*
 * default exception handler will convert OS specific register frame format
 * into a standard GDB frame layout used by both the GDB server and coredumper.
 * We need to minimize stack usage and cannot do heap allocation, thus we
 * use some storage in the .data segment.
 */
static struct regfile regs;

IRAM NOINSTR static void handle_exception(struct regfile *regs) {
  xthal_set_intenable(0);

#if defined(ESP_COREDUMP) && !defined(ESP_COREDUMP_NOAUTO)
  fprintf(stderr, "Dumping core\n");
  esp_dump_core(regs);
#else
  printf("if you want to dump core, type 'y'");
#ifdef ESP_GDB_SERVER
  printf(", or ");
#endif
#endif
#ifdef ESP_GDB_SERVER
  printf("connect with gdb now\n");
#endif

#if defined(ESP_COREDUMP_NOAUTO) || defined(ESP_GDB_SERVER)
  {
    int ch;
    while ((ch = blocking_read_uart())) {
      if (ch == 'y') {
        esp_dump_core(regs);
      } else if (ch == '$') {
        /* we got a GDB packet, speed up retransmission by nacking */
        printf("-");
        gdb_server(regs);
        break;
      }
    }
  }
#endif
}

/*
 * xtos low level exception handler (in rom)
 * populates an xtos_regs structure with (most) registers
 * present at the time of the exception and passes it to the
 * high-level handler.
 *
 * Note that the a1 (sp) register is clobbered (bug? necessity?),
 * however the original stack pointer can be inferred from the address
 * of the saved registers area, since the exception handler uses the same
 * user stack. This might be different in other execution modes on the
 * quite variegated xtensa platform family, but that's how it works on ESP8266.
 */
IRAM NOINSTR void esp_exception_handler(struct xtensa_stack_frame *frame) {
  uint32_t cause = RSR(EXCCAUSE);
  uint32_t vaddr = RSR(EXCVADDR);
  fprintf(stderr, "\nTrap %d: pc=%p va=%p\n", cause, (void *) frame->pc,
          (void *) vaddr);
  memcpy(&regs.a[2], frame->a, sizeof(frame->a));

  regs.a[0] = frame->a0;
  regs.a[1] = (uint32_t) frame + ESP_EXC_SP_OFFSET;
  regs.pc = frame->pc;
  regs.sar = frame->sar;
  regs.ps = frame->ps;
  regs.litbase = RSR(LITBASE);

  handle_exception(&regs);

  fprintf(stderr, "rebooting\n");
  fs_flush_stderr();

  /*
   * Documented `system_restart` does a lot of things (cleanup) which (seems)
   * cannot be done from exc handler and only after cleanup it calls
   * `system_restart_local`.
   */
  system_restart_local();
}

NOINSTR void esp_exception_handler_init() {
#if defined(ESP_FLASH_BYTES_EMUL) || defined(ESP_GDB_SERVER) || \
    defined(ESP_COREDUMP)

  char causes[] = {EXCCAUSE_ILLEGAL,          EXCCAUSE_INSTR_ERROR,
                   EXCCAUSE_LOAD_STORE_ERROR, EXCCAUSE_DIVIDE_BY_ZERO,
                   EXCCAUSE_UNALIGNED,        EXCCAUSE_INSTR_PROHIBITED,
                   EXCCAUSE_LOAD_PROHIBITED,  EXCCAUSE_STORE_PROHIBITED};
  int i;
  for (i = 0; i < (int) sizeof(causes); i++) {
    _xtos_set_exception_handler(causes[i], esp_exception_handler);
  }

#ifdef ESP_FLASH_BYTES_EMUL
  /*
   * registers exception handlers that allow reading arbitrary data from
   * flash
   */
  flash_emul_init();
#endif

#endif
}

void esp_print_reset_info() {
  struct rst_info *ri = system_get_rst_info();
  const char *reason_str;
  int print_exc_info = 0;
  switch (ri->reason) {
    case REASON_DEFAULT_RST:
      reason_str = "power on";
      break;
    case REASON_WDT_RST:
      reason_str = "HW WDT";
      print_exc_info = 1;
      break;
    case REASON_EXCEPTION_RST:
      reason_str = "exception";
      print_exc_info = 1;
      break;
    case REASON_SOFT_WDT_RST:
      reason_str = "SW WDT";
      print_exc_info = 1;
      break;
    case REASON_SOFT_RESTART:
      reason_str = "soft reset";
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      reason_str = "deep sleep wake";
      break;
    case REASON_EXT_SYS_RST:
      reason_str = "sys reset";
      break;
    default:
      reason_str = "???";
      break;
  }
  LOG(LL_INFO, ("Reset cause: %u (%s)", ri->reason, reason_str));
  if (print_exc_info) {
    LOG(LL_INFO,
        ("Exc info: cause=%u epc1=0x%08x epc2=0x%08x epc3=0x%08x vaddr=0x%08x "
         "depc=0x%08x",
         ri->exccause, ri->epc1, ri->epc2, ri->epc3, ri->excvaddr, ri->depc));
  }
}
