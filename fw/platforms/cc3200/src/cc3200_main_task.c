#include "fw/platforms/cc3200/src/cc3200_main_task.h"

#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

#include "common/platform.h"

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

#define CB_ADDR_MASK 0xe0000000
#define CB_ADDR_PREFIX 0x20000000

#define PROMPT_CHAR_EVENT 0
#define INVOKE_CB_EVENT 1
struct sj_event {
  /*
   * We exploit the fact that all callback addresses have to be in SRAM and
   * start with CB_ADDR_PREFIX and use the 3 upper bits to store message type.
   */
  unsigned type : 3;
  unsigned cb : 29;
  void *data;
};

struct v7 *s_v7;
OsiMsgQ_t s_main_queue;
extern const char *build_id;

struct v7 *init_v7(void *stack_base) {
  struct v7_create_opts opts;

  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
  opts.c_stack_base = stack_base;

  return v7_create_opt(opts);
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

#ifndef CS_DISABLE_JS
static void uart_int() {
  struct sj_event e = {.type = PROMPT_CHAR_EVENT, .data = NULL};
  MAP_UARTIntClear(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  MAP_UARTIntDisable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
  osi_MsgQWrite(&s_main_queue, &e, OSI_NO_WAIT);
}

void sj_prompt_init_hal(struct v7 *v7) {
  (void) v7;
  osi_InterruptRegister(CONSOLE_UART_INT, uart_int, INT_PRIORITY_LVL_1);
  MAP_UARTIntEnable(CONSOLE_UART, UART_INT_RX | UART_INT_RT);
}
#endif

static int sj_init() {
  struct v7 *v7 = s_v7;

  cs_log_set_level(LL_INFO);
  LOG(LL_INFO, ("Mongoose IoT Firmware %s", build_id));
  LOG(LL_INFO,
      ("RAM: %d total, %d free", sj_get_heap_size(), sj_get_free_heap_size()));

  int r = start_nwp();
  if (r < 0) {
    LOG(LL_ERROR, ("Failed to start NWP: %d", r));
    return 0;
  }

  osi_MsgQCreate(&s_main_queue, "V7", sizeof(struct sj_event), 32 /* len */);

  int boot_cfg_idx = get_active_boot_cfg_idx();
  if (boot_cfg_idx < 0) return 0;
  struct boot_cfg boot_cfg;
  if (read_boot_cfg(boot_cfg_idx, &boot_cfg) < 0) return 0;

  LOG(LL_INFO, ("Boot cfg %d: 0x%llx, 0x%u, %s @ 0x%08x, %s %p", boot_cfg_idx,
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
    return 0;
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

  LOG(LL_INFO, ("%s init done, RAM: %d free", "Sys", sj_get_free_heap_size()));

  if (!sj_app_init(v7)) {
    LOG(LL_ERROR, ("App init failed"));
    abort();
  }
  LOG(LL_INFO, ("%s init done, RAM: %d free", "App", sj_get_free_heap_size()));

  if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    boot_cfg.seq = saved_seq;
    commit_update(boot_cfg_idx, &boot_cfg);
  }

#ifndef CS_DISABLE_JS
  sj_prompt_init(v7);
#endif
  return 1;
}

void main_task(void *arg) {
  (void) arg;

  if (!sj_init()) {
    LOG(LL_ERROR, ("Init failed"));
    sj_system_restart(0);
    return;
  }

  while (1) {
    struct sj_event e;
    mongoose_poll(0);
    if (osi_MsgQRead(&s_main_queue, &e, V7_POLL_LENGTH_MS) != OSI_OK) continue;
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
#endif
      case INVOKE_CB_EVENT: {
        cb_t cb = (cb_t)(e.cb | CB_ADDR_PREFIX);
        cb(e.data);
        break;
      }
    }
  }
}

void invoke_cb(cb_t cb, void *arg) {
  assert(cb & CB_ADDR_MASK == 0);
  struct sj_event e;
  e.type = INVOKE_CB_EVENT;
  e.cb = (unsigned) cb;
  e.data = arg;
  osi_MsgQWrite(&s_main_queue, &e, OSI_WAIT_FOREVER);
}
