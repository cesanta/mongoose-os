/* simple wrapper around the stlink_flash_write function */

// TODO - this should be done as just a simple flag to the st-util command
// line...

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "stlink.h"
#include "usb.h"

static stlink_t *connected_stlink = NULL;

struct flash_opts {
  const char *devname;
  uint8_t serial[16];
  const char *filename;
  stm32_addr_t addr;
  size_t size;
  int reset;
  int log_level;
  int format;
};

static void cleanup(int signum) {
  (void) signum;

  if (connected_stlink) {
    /* Switch back to mass storage mode before closing. */
    stlink_run(connected_stlink);
    stlink_exit_debug_mode(connected_stlink);
    stlink_close(connected_stlink);
  }

  exit(1);
}

static void usage(void) {
  puts("<serial> <path>");
}

int main(int ac, char **av) {
  stlink_t *sl = NULL;
  struct flash_opts o;
  int err = -1;
  uint8_t *mem = NULL;

  o.size = 0;
  o.reset = 1;
  o.filename = av[2];
  o.addr = 0x8000000;

  int j = (int) strlen(av[1]);
  int length = j / 2;  // the length of the destination-array
  if (j % 2 != 0) return -1;

  for (size_t k = 0; j >= 0 && k < sizeof(o.serial); ++k, j -= 2) {
    char buffer[3] = {0};
    memcpy(buffer, av[1] + j, 2);
    o.serial[length - k] = (uint8_t) strtol(buffer, NULL, 16);
  }

  sl = stlink_open_usb(1, (char *) o.serial);

  if (sl == NULL) return -1;

  connected_stlink = sl;
  signal(SIGINT, &cleanup);
  signal(SIGTERM, &cleanup);
  signal(SIGSEGV, &cleanup);

  if (stlink_current_mode(sl) == STLINK_DEV_DFU_MODE) {
    if (stlink_exit_dfu_mode(sl)) {
      printf("Failed to exit DFU mode\n");
      goto on_error;
    }
  }

  if (stlink_current_mode(sl) != STLINK_DEV_DEBUG_MODE) {
    if (stlink_enter_swd_mode(sl)) {
      printf("Failed to enter SWD mode\n");
      goto on_error;
    }
  }

  if (o.reset) {
    if (stlink_jtag_reset(sl, 2)) {
      printf("Failed to reset JTAG\n");
      goto on_error;
    }

    if (stlink_reset(sl)) {
      printf("Failed to reset device\n");
      goto on_error;
    }
  }

  // Core must be halted to use RAM based flashloaders
  if (stlink_force_debug(sl)) {
    printf("Failed to halt the core\n");
    goto on_error;
  }

  if (stlink_status(sl)) {
    printf("Failed to get Core's status\n");
    goto on_error;
  }

  size_t size = 0;

  if ((o.addr >= sl->flash_base) &&
      (o.addr < sl->flash_base + sl->flash_size)) {
    err = stlink_fwrite_flash(sl, o.filename, o.addr);
    if (err == -1) {
      printf("stlink_fwrite_flash() == -1\n");
      goto on_error;
    }
  } else {
    err = -1;
    printf("Unknown memory region\n");
    goto on_error;
  }

  if (o.reset) {
    stlink_jtag_reset(sl, 2);
    stlink_reset(sl);
  }

  /* success */
  err = 0;

on_error:
  stlink_exit_debug_mode(sl);
  stlink_close(sl);
  free(mem);

  return err;
}
