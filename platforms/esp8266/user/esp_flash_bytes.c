#ifdef ESP_FLASH_BYTES_EMUL

#ifndef RTOS_SDK
#include "user_interface.h"
#else
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <c_types.h>
#include <stdio.h>
#include <freertos/xtensa_rtos.h>

/*
 * If we leave this out it crashes, WTF?!
 * It's weird since printfs here are only invoked on exception
 * and by defining this macro we boot fine without exceptions
 */
#define printf printf_broken

#endif

#include <stdio.h>
#include "esp_exc.h"
#include "xtensa/corebits.h"
#include "esp_hw.h"
#include "esp_missing_includes.h"

/*
 * Only the L32I, L32I.N, and L32R load instructions can access InstRAM
 * and InstROM locations. Any other memory access with in those ranges will
 * cause EXCCAUSE_LOAD_STORE_ERROR.
 *
 * This exception handler emulates the L8UI, L16SI and L16UI loads
 * by decoding the offending instruction, performing the necessary
 * 32 bit accesses around the desired address, sets the result in the
 * target register, patches up the program counter to point to the next
 * instruction and resumes execution.
 */
FAST void flash_emul_exception_handler(struct xtensa_stack_frame *frame)
    __attribute__((no_instrument_function));
void flash_emul_exception_handler(struct xtensa_stack_frame *frame) {
  uint32_t vaddr = RSR(EXCVADDR);

  /*
   * Xtensa instructions are 24-bit wide.
   * Among the load instructions, only the 32-bit variety
   * can use the 16-bit encoding with available with the
   * Code Density option (enabled on lx106).
   *
   * All full-width load and store instructions follow the encoding
   * called LSAI: opcode (lowest 4 bits) set to 0x2.
   * Furthermore they all use the RRI8 encoding:
   *
   * |23            16|15   12|11    8|7     4|3     0|
   * |----------------+-------+-------+-------+-------|
   * |     imm8       |   r   |   as  |   at  |0 0 1 0|
   *
   * The actual instruction depends on the value of r.
   *
   * Aligned 32-bit flash data access doesn't require emulation,
   * however when using this feature to implement mmap via
   * faults to an intentionally unaccessible areas, all loads
   * have to be emulated including 32-bit.
   *
   * Narrow l32i.n instruction encoding is:
   *
   * |15   12|11    8|7     4|3     0|
   * |-------+-------+-------+-------|
   * | imm4  |   as  |   at  |1 0 0 0|
   *
   * Since vaddr already contains the effective address we don't have to fetch
   * the value of the at register nor decode the third byte of the instruction.
   */
  uint32_t instr = read_unaligned_byte((uint8_t *) frame->pc) |
                   (read_unaligned_byte((uint8_t *) frame->pc + 1) << 8);
  uint8_t at = (instr >> 4) & 0xf;
  uint32_t val = 0;

  //  printf("READING INSTRUCTION FROM %p\n", (void *) frame->pc);
  //  printf("AT register %d\n", at);

  if ((instr & 0xf00f) == 0x2) {
    /* l8ui at, as, imm       r = 0 */
    val = read_unaligned_byte((uint8_t *) vaddr);
  } else if ((instr & 0x700f) == 0x1002) {
    /*
     * l16ui at, as, imm      r = 1
     * l16si at, as, imm      r = 9
     */
    val = read_unaligned_byte((uint8_t *) vaddr) |
          read_unaligned_byte((uint8_t *) vaddr + 1) << 8;
    if (instr & 0x8000) val = (int16_t) val;
  } else if ((instr & 0xf00f) == 0x2002 || (instr & 0xf) == 0x8) {
    /*
     * l32i   at, as, imm      r = 2
     * l32i.n at, as, imm
     */
    /*
     * TODO(mkm): provide fast code path for aligned access since
     * all mmap 32-bit loads will be aligned.
     */
    val = read_unaligned_byte((uint8_t *) vaddr) |
          read_unaligned_byte((uint8_t *) vaddr + 1) << 8 |
          read_unaligned_byte((uint8_t *) vaddr + 2) << 16 |
          read_unaligned_byte((uint8_t *) vaddr + 3) << 24;
    if ((instr & 0xf) == 0x8) {
      frame->pc -= 1; /* this instruction is only 2 bytes wide */
    }
  } else {
    printf("cannot emulate flash mem instr at pc = %p\n", (void *) frame->pc);
    esp_exception_handler(frame);
    return;
  }

#ifndef RTOS_SDK
  /* a0 and a1 are never used as scratch registers */
  frame->a[at - 2] = val;
#else
  frame->a[at] = val;
#endif
  frame->pc += 3;

#ifdef RTOS_SDK
  taskYIELD();
#endif

  // printf("PC FIXED UP RETURNING to %p\n", (void *) frame->pc);
}

#ifndef RTOS_SDK
void flash_emul_init() {
  _xtos_set_exception_handler(EXCCAUSE_LOAD_STORE_ERROR,
                              flash_emul_exception_handler);
  /* for mmap */
  _xtos_set_exception_handler(EXCCAUSE_LOAD_PROHIBITED,
                              flash_emul_exception_handler);
}
#else
void flash_emul_init() {
}
#endif /* RTOS_SDK */

#endif /* ESP_FLASH_BYTES_EMUL */
