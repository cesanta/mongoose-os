#ifndef ESP_EXC_INCLUDED
#define ESP_EXC_INCLUDED

/*
 * the saved registers begin at a fixed position in the xtos
 * low-level exception handler. I don't know if 0x100 it's just an
 * artifact of the actual xtos build ESP8266EX is using (although this nice
 * round number looks deliberate). The exception handler is burned on rom
 * so it should work on future SDK updates, but not necessarily on future
 * revisions of the chip.
 */
#define ESP_EXC_SP_OFFSET 0x100

/*
 * Addresses in this range are guaranteed to be readable without faulting.
 * Contains ranges that are unmapped but innocuous.
 *
 * Putative ESP8266 memory map at:
 * https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map
 */
#define ESP_LOWER_VALID_ADDRESS 0x20000000
#define ESP_UPPER_VALID_ADDRESS 0x60000000

#ifndef RTOS_SDK

/*
 * Constructed by xtos.
 *
 * There is a UserFrame structure in
 * ./esp_iot_rtos_sdk/extra_include/xtensa/xtruntime-frames.h
 */
struct xtensa_stack_frame {
  uint32_t pc; /* instruction causing the trap */
  uint32_t ps;
  uint32_t sar;
  uint32_t vpri;  /* current xtos virtual priority */
  uint32_t a0;    /* when __XTENSA_CALL0_ABI__ is true */
  uint32_t a[16]; /* a2 - a15 */
};

#else

/* from <freertos/xtensa_context.h> */
struct xtensa_stack_frame {
  uint32_t exit;  /* (offset 0) exit point for dispatch */
  uint32_t pc;    /* return address */
  uint32_t ps;    /* at level 1 ps.excm is set here */
  uint32_t a[16]; /* a[1] is stack ptr before interrupt */
  uint32_t sar;
};

#endif

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

void esp_exception_handler(struct xtensa_stack_frame *frame);
void esp_exception_handler_init();

#endif /* ESP_EXC_INCLUDED */
