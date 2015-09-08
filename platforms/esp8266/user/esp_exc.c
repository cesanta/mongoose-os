#include <stdio.h>
#include <string.h>
#include <ets_sys.h>
#include <xtensa/corebits.h>
#include <stdint.h>
#include "esp_exc.h"
#include "esp_gdb.h"
#include "esp_coredump.h"
#include "esp_flash_bytes.h"
#include "v7_esp_hw.h"
#include "esp_uart.h"
#include "esp_missing_includes.h"
#include "v7_esp.h"
#include "base64.h"
#include "esp_uart.h"

#ifndef RTOS_SDK

#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <mem.h>

#else

#include <c_types.h>
#include <xtensa/xtruntime.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void system_soft_wdt_feed();

#endif /* RTOS_SDK */

/*
 * default exception handler will convert OS specific register frame format
 * into a standard GDB frame layout used by both the GDB server and coredumper.
 * We need to minimize stack usage and cannot do heap allocation, thus we
 * use some storage in the .data segment.
 */
static struct regfile regs;

static void handle_exception(struct regfile *regs) {
  xthal_set_intenable(0);

#ifdef ESP_COREDUMP
  esp_dump_core(regs);
#endif
#ifdef ESP_GDB_SERVER
  gdb_server(regs);
#endif
}

#ifndef RTOS_SDK
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
FAST void esp_exception_handler(struct xtos_saved_regs *frame) {
  uint32_t cause = RSR(EXCCAUSE);
  uint32_t vaddr = RSR(EXCVADDR);
  printf("\nTrap %d: pc=%p va=%p\n", cause, (void *) frame->pc, (void *) vaddr);
  memcpy(&regs.a[2], frame->a, sizeof(frame->a));

  regs.a[0] = frame->a0;
  regs.a[1] = (uint32_t) frame + ESP_EXC_SP_OFFSET;
  regs.pc = frame->pc;
  regs.sar = frame->sar;
  regs.ps = frame->ps;
  regs.litbase = RSR(LITBASE);

  handle_exception(&regs);
  _ResetVector();
}

#else /* !RTOS_SDK */

void __wrap_user_fatal_exception_handler(int cause) {
  xTaskHandle th = xTaskGetCurrentTaskHandle();
  /*
   * The task handle should be opaque but we'll assume here it's
   * a pointer to a tskTaskControlBlock structure, whose first field is:
   *
   * volatile portSTACK_TYPE *pxTopOfStack;
   *
   * It's documentation at least guarantees that it's the first element.
   * Before an exception handler is invoked extensa rtos will save the regiters
   * in a stack frame. We captured the structure of that frame in
   * xtensa_rtos_stack_frame.
   */
  struct xtensa_rtos_stack_frame *frame =
      (struct xtensa_rtos_stack_frame *) *(void **) th;

  uint32_t vaddr = RSR(EXCVADDR);
  printf("\nTrap %d: pc=%p va=%p\n", cause, (void *) frame->pc, (void *) vaddr);

  memcpy(&regs.a[0], frame->a, sizeof(frame->a));
  regs.pc = frame->pc;
  regs.sar = frame->sar;
  regs.ps = frame->ps;
  regs.litbase = RSR(LITBASE);

  handle_exception(&regs);

  /* TODO(mkm): call system exception vector */
  while (1) {
    system_soft_wdt_feed();
  }
}

#endif /* !RTOS_SDK */

void esp_exception_handler_init() {
#if defined(ESP_FLASH_BYTES_EMUL) || defined(ESP_GDB_SERVER) || \
    defined(ESP_COREDUMP)

/*
 * The RTOS build intercepts all user exceptions with
 * __wrap_user_fatal_exception_handler
 */
#ifndef RTOS_TODO
  char causes[] = {EXCCAUSE_ILLEGAL,          EXCCAUSE_INSTR_ERROR,
                   EXCCAUSE_LOAD_STORE_ERROR, EXCCAUSE_DIVIDE_BY_ZERO,
                   EXCCAUSE_UNALIGNED,        EXCCAUSE_INSTR_PROHIBITED,
                   EXCCAUSE_LOAD_PROHIBITED,  EXCCAUSE_STORE_PROHIBITED};
  int i;
  for (i = 0; i < (int) sizeof(causes); i++) {
    _xtos_set_exception_handler(causes[i], esp_exception_handler);
  }
#endif

#ifdef ESP_FLASH_BYTES_EMUL
  /*
   * registers exception handlers that allow reading arbitrary data from flash
   */
  flash_emul_init();
#endif

#endif
}
