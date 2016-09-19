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

#include "fw/src/mg_app.h"
#include "fw/src/mg_hal.h"
#include "fw/src/mg_init.h"
#include "fw/src/mg_init_js.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_prompt.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_uart.h"
#include "fw/src/mg_updater_clubby.h"

#ifdef MG_ENABLE_JS
#include "v7/v7.h"
#endif

#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_crypto.h"
#include "fw/platforms/cc3200/src/cc3200_fs.h"
#include "fw/platforms/cc3200/src/cc3200_updater.h"

#define CB_ADDR_MASK 0xe0000000
#define CB_ADDR_PREFIX 0x20000000

struct mg_event {
  cb_t cb;
  void *arg;
};

OsiMsgQ_t s_main_queue;
extern const char *build_id;

#ifdef MG_ENABLE_JS
struct v7 *s_v7;

struct v7 *init_v7(void *stack_base) {
  struct v7_create_opts opts;

#ifdef V7_THAW
  opts.object_arena_size = 85;
  opts.function_arena_size = 16;
  opts.property_arena_size = 100;
#else
  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
#endif
  opts.c_stack_base = stack_base;

  return v7_create_opt(opts);
}
#endif

/* It may not be the best source of entropy, but it's better than nothing. */
static void cc3200_srand(void) {
  uint32_t r = 0, *p;
  for (p = (uint32_t *) 0x20000000; p < (uint32_t *) 0x20040000; p++) r ^= *p;
  srand(r);
}

int start_nwp(void) {
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
  cc3200_srand();
  return 0;
}

#ifdef MG_ENABLE_JS
void mg_prompt_init_hal(void) {
}
#endif

enum cc3200_init_result {
  CC3200_INIT_OK = 0,
  CC3200_INIT_FAILED_TO_START_NWP = -100,
  CC3200_INIT_FAILED_TO_READ_BOOT_CFG = -101,
  CC3200_INIT_FS_INIT_FAILED = -102,
  CC3200_INIT_MG_INIT_FAILED = -103,
  CC3200_INIT_MG_INIT_JS_FAILED = -105,
  CC3200_INIT_UART_INIT_FAILED = -106,
};

static enum cc3200_init_result cc3200_init(void *arg) {
  mongoose_init();
  if (MG_DEBUG_UART >= 0) {
    struct mg_uart_config *ucfg = mg_uart_default_config();
    ucfg->baud_rate = MG_DEBUG_UART_BAUD_RATE;
    if (mg_uart_init(MG_DEBUG_UART, ucfg, NULL, NULL) == NULL) {
      return CC3200_INIT_UART_INIT_FAILED;
    }
  }

  LOG(LL_INFO, ("Mongoose IoT Firmware %s", build_id));
  LOG(LL_INFO,
      ("RAM: %d total, %d free", mg_get_heap_size(), mg_get_free_heap_size()));

  int r = start_nwp();
  if (r < 0) {
    LOG(LL_ERROR, ("Failed to start NWP: %d", r));
    return CC3200_INIT_FAILED_TO_START_NWP;
  }

  int boot_cfg_idx = get_active_boot_cfg_idx();
  struct boot_cfg boot_cfg;
  if (boot_cfg_idx < 0 || read_boot_cfg(boot_cfg_idx, &boot_cfg) < 0) {
    return CC3200_INIT_FAILED_TO_READ_BOOT_CFG;
  }

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

  r = cc3200_fs_init(boot_cfg.fs_container_prefix);
  if (r < 0) {
    LOG(LL_ERROR, ("FS init error: %d", r));
    if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
      revert_update(boot_cfg_idx, &boot_cfg);
    }
    return CC3200_INIT_FS_INIT_FAILED;
  }

  if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    LOG(LL_INFO, ("Applying update"));
    if (apply_update(boot_cfg_idx, &boot_cfg) < 0) {
      revert_update(boot_cfg_idx, &boot_cfg);
    }
  }

  enum mg_init_result ir = mg_init();
  if (ir != MG_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "MG", ir));
    return CC3200_INIT_MG_INIT_FAILED;
  }

#ifdef MG_ENABLE_JS
  struct v7 *v7 = s_v7 = init_v7(&arg);

  ir = mg_init_js_all(v7);
  if (ir != MG_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "JS", ir));
    return CC3200_INIT_MG_INIT_JS_FAILED;
  }
#endif

  LOG(LL_INFO, ("Init done, RAM: %d free", mg_get_free_heap_size()));

  if (boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    boot_cfg.seq = saved_seq;
    commit_update(boot_cfg_idx, &boot_cfg);
#ifdef MG_ENABLE_CLUBBY
    clubby_updater_finish(0);
#endif
  } else {
#ifdef MG_ENABLE_CLUBBY
    /*
     * If there is no update reply state, this will just be ignored.
     * But if there is, then update was rolled back and reply will be sent.
     */
    clubby_updater_finish(-1);
#endif
  }

#ifdef MG_ENABLE_JS
  mg_prompt_init(v7, get_cfg()->debug.stdout_uart);
#endif
  return CC3200_INIT_OK;
}

void mongoose_poll_cb(void *arg);

void main_task(void *arg) {
  struct mg_event e;
  osi_MsgQCreate(&s_main_queue, "main", sizeof(e), 32 /* len */);

  enum cc3200_init_result r = cc3200_init(NULL);
  if (r != CC3200_INIT_OK) {
    LOG(LL_ERROR, ("Init failed: %d", r));
    mg_system_restart(0);
    return;
  }

  while (1) {
    mongoose_poll(0);
    cc3200_fs_flush();
    if (osi_MsgQRead(&s_main_queue, &e, V7_POLL_LENGTH_MS) == OSI_OK) {
      e.cb(e.arg);
    }
  }
}

bool invoke_cb(cb_t cb, void *arg) {
  struct mg_event e = {.cb = cb, .arg = arg};
  return (osi_MsgQWrite(&s_main_queue, &e, OSI_NO_WAIT) == OSI_OK);
}
