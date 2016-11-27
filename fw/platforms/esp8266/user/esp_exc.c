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

#include "fw/src/miot_hal.h"

#include "esp_coredump.h"
#include "esp_flash_bytes.h"
#include "esp_fs.h"
#include "esp_gdb.h"
#include "esp_hw.h"
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

/* This is system_restart_local, renamed so we can intercept WDT. */
void system_restart_local_sdk();

IRAM NOINSTR static void handle_exception(int cause, struct regfile *regs) {
  xthal_set_intenable(0);

#if defined(ESP_COREDUMP) && !defined(ESP_COREDUMP_NOAUTO)
  esp_dump_core(cause, regs);
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

NOINSTR void regs_from_xtos_frame(UserFrame *frame) {
  regs.a[0] = frame->a0;
  regs.a[1] = (uint32_t) frame + ESF_TOTALSIZE;
  regs.a[2] = frame->a2;
  regs.a[3] = frame->a3;
  regs.a[4] = frame->a4;
  regs.a[5] = frame->a5;
  regs.a[6] = frame->a6;
  regs.a[7] = frame->a7;
  regs.a[8] = frame->a8;
  regs.a[9] = frame->a9;
  regs.a[10] = frame->a10;
  regs.a[11] = frame->a11;
  regs.a[12] = frame->a12;
  regs.a[13] = frame->a13;
  regs.a[14] = frame->a14;
  regs.a[15] = frame->a15;
  regs.pc = frame->pc;
  regs.sar = frame->sar;
  regs.litbase = RSR(LITBASE);
  regs.sr176 = 0;
  regs.sr208 = 0;
  regs.ps = frame->ps;
}

NOINSTR void esp_exception_handler(UserFrame *frame) {
  /* Not soted in the frame's fields for some reason. */
  uint32_t cause = RSR(EXCCAUSE);
  uint32_t vaddr = RSR(EXCVADDR);
  fprintf(stderr, "\nTrap %u: pc=%p va=%p\n", cause, (void *) frame->pc,
          (void *) vaddr);
  regs_from_xtos_frame(frame);
  handle_exception(cause, &regs);

  fprintf(stderr, "rebooting\n");
  fs_flush_stderr();

  /*
   * Documented `system_restart` does a lot of things (cleanup) which (seems)
   * cannot be done from exc handler and only after cleanup it calls
   * `system_restart_local`.
   */
  system_restart_local_sdk();
}

NOINSTR void esp_exception_handler_init(void) {
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

void esp_print_reset_info(void) {
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

NOINSTR void system_restart_local(void) {
  struct rst_info ri;
  system_rtc_mem_read(0, &ri, sizeof(ri));
  if (ri.reason == REASON_SOFT_WDT_RST) {
    LOG(LL_INFO,
        ("WDT reset, info: exccause=%u epc1=0x%08x epc2=0x%08x epc3=0x%08x "
         "vaddr=0x%08x depc=0x%08x",
         ri.exccause, ri.epc1, ri.epc2, ri.epc3, ri.excvaddr, ri.depc));
#ifdef ESP_COREDUMP
    /* So, here's how we got here:
     *  0) system_restart_local (that's us!) - 64 byte frame.
     *  1) pp_soft_wdt_feed_local - 32 bytes.
     *  2) static local function in the SDK, probably wDev_something - 16 bytes.
     *  3) wDev_ProcessFiq - 48 bytes.
     *  4) _xtos_l1int_handler - in ROM, ret 0x4000050c, no stack of its own
     *  5) XTOS user vector mode exception stack frame,
     *     aka struct UserFrame - 256 bytes (ESF_TOTALSIZE)
     *  6) ... pre-interrupt stack.
     *
     * Needless to say, stack frame sizes are less than reliable. So we try to
     * make our code more robust in the face of SDK changes by:
     *  1) Searching the stack for 0x4000050c first. This is the return address
     *     of the int handler in ROM, it will not change. This we will find in
     *     wDev_ProcessFiq's stack frame (note: it is compiled with something
     *     other than GCC and return address is not at the end of the frame but
     *     in the middle).
     *  2) Searching for the epc1 value from reset info, which is the PC value
     *     which is the first field of the UserFrame struct. wDev_ProcessFiq
     *     seems to be something generic enough that it doesn't care about PC,
     *     so it's unlikely that it will appear on its stack.
     * We assume that stack frames are 32-bit aligned, which seems reasonabe.
     */
    uint32_t *sp = (uint32_t *) &ri;
    while (sp < (uint32_t *) 0x40000000 && *sp != 0x4000050c) sp++;
    while (sp < (uint32_t *) 0x40000000 && *sp != ri.epc1) sp++;
    if (sp < (uint32_t *) 0x40000000) {
      regs_from_xtos_frame((UserFrame *) sp);
      esp_dump_core(100 /* not really an exception */, &regs);
    }
#endif
  }
#ifdef MIOT_STOP_ON_EXCEPTION
  fprintf(stderr, "NOT rebooting.\n");
  fs_flush_stderr();
  while (1) {
    miot_wdt_feed();
  }
#else
  fprintf(stderr, "rebooting\n");
  fs_flush_stderr();
  system_restart_local_sdk();
#endif
}
