#ifndef V7_GDB_INCLUDED
#define V7_GDB_INCLUDED

/*
 * the saved registers begin at a fixed position in the xtos
 * low-level exception handler. I don't know if 0x100 it's just an
 * artifact of the actual xtos build ESP8266EX is using (although this nice
 * round number looks deliberate). The exception handler is burned on rom
 * so it should work on future SDK updates, but not necessarily on future
 * revisions of the chip.
 */
#define V7_GDB_SP_OFFSET 0x100

/*
 * Addresses in this range are guaranteed to be readable without faulting.
 * Contains ranges that are unmapped but innocuous.
 *
 * Putative ESP8266 memory map at:
 * https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map
 */
#define ESP_LOWER_VALID_ADDRESS 0x20000000
#define ESP_UPPER_VALID_ADDRESS 0x60000000

/*
 * Constructed by xtos.
 *
 * There is a UserFrame structure in
 * ./esp_iot_rtos_sdk/extra_include/xtensa/xtruntime-frames.h
 */
struct xtos_saved_regs {
  uint32_t pc; /* instruction causing the trap */
  uint32_t ps;
  uint32_t sar;
  uint32_t vpri;  /* current xtos virtual priority */
  uint32_t a0;    /* when __XTENSA_CALL0_ABI__ is true */
  uint32_t a[16]; /* a2 - a15 */
};

/*
 * Register file in the format lx106 gdb port expects it.
 *
 * Inspired by gdb/regformats/reg-xtensa.dat from
 * https://github.com/jcmvbkbc/crosstool-NG/blob/lx106-g%2B%2B/overlays/xtensa_lx106.tar
 */
struct regfile {
  uint32_t a[16];
  uint32_t pc;
  uint32_t sar;
  uint32_t litbase;
  uint32_t sr176;
  uint32_t sr208;
  uint32_t ps;
};

void gdb_init();
void gdb_exception_handler(struct xtos_saved_regs *frame);

#endif /* V7_GDB_INCLUDED */
