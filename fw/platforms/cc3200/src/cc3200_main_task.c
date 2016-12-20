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

#include "fw/src/miot_app.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_prompt.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_uart.h"
#include "fw/src/miot_updater_common.h"

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#endif

#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_crypto.h"
#include "fw/platforms/cc3200/src/cc3200_fs.h"
#include "fw/platforms/cc3200/src/cc3200_fs_spiffs_container.h"

#define CB_ADDR_MASK 0xe0000000
#define CB_ADDR_PREFIX 0x20000000

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

int g_boot_cfg_idx;
struct boot_cfg g_boot_cfg;

struct miot_event {
  miot_cb_t cb;
  void *arg;
};

OsiMsgQ_t s_main_queue;

#if MIOT_ENABLE_JS
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

#if MIOT_ENABLE_JS
void miot_prompt_init_hal(void) {
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
  CC3200_INIT_UPDATE_FAILED = -107,
};

static enum cc3200_init_result cc3200_init(void *arg) {
  mongoose_init();
  if (cc3200_console_init() != MIOT_INIT_OK) {
    return CC3200_INIT_UART_INIT_FAILED;
  }

  if (strcmp(MIOT_APP, "mongoose-iot") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MIOT_APP, build_version, build_id));
  }
  LOG(LL_INFO,
      ("Mongoose IoT Firmware %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("RAM: %d total, %d free", miot_get_heap_size(),
                miot_get_free_heap_size()));

  int r = start_nwp();
  if (r < 0) {
    LOG(LL_ERROR, ("Failed to start NWP: %d", r));
    return CC3200_INIT_FAILED_TO_START_NWP;
  }

  g_boot_cfg_idx = get_active_boot_cfg_idx();
  if (g_boot_cfg_idx < 0 || read_boot_cfg(g_boot_cfg_idx, &g_boot_cfg) < 0) {
    return CC3200_INIT_FAILED_TO_READ_BOOT_CFG;
  }

  LOG(LL_INFO, ("Boot cfg %d: 0x%llx, 0x%u, %s @ 0x%08x, %s", g_boot_cfg_idx,
                g_boot_cfg.seq, g_boot_cfg.flags, g_boot_cfg.app_image_file,
                g_boot_cfg.app_load_addr, g_boot_cfg.fs_container_prefix));

  if (g_boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    /* Tombstone the current config. If anything goes wrong between now and
     * commit, next boot will use the old one. */
    uint64_t saved_seq = g_boot_cfg.seq;
    g_boot_cfg.seq = BOOT_CFG_TOMBSTONE_SEQ;
    write_boot_cfg(&g_boot_cfg, g_boot_cfg_idx);
    g_boot_cfg.seq = saved_seq;
  }

  r = cc3200_fs_init(g_boot_cfg.fs_container_prefix);
  if (r < 0) {
    LOG(LL_ERROR, ("FS init error: %d", r));
    return CC3200_INIT_FS_INIT_FAILED;
  } else {
    /*
     * We aim to maintain at most 3 FS containers at all times.
     * Delete inactive FS container in the inactive boot configuration.
     */
    struct boot_cfg cfg;
    int inactive_idx = (g_boot_cfg_idx == 0 ? 1 : 0);
    if (read_boot_cfg(inactive_idx, &cfg) >= 0) {
      fs_delete_inactive_container(cfg.fs_container_prefix);
    }
  }

#if MIOT_ENABLE_UPDATER
  if (g_boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    LOG(LL_INFO, ("Applying update"));
    r = miot_upd_apply_update();
    if (r < 0) {
      LOG(LL_ERROR, ("Failed to apply update: %d", r));
      return CC3200_INIT_UPDATE_FAILED;
    }
  }
#endif

  enum miot_init_result ir = miot_init();
  if (ir != MIOT_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "MG", ir));
    return CC3200_INIT_MG_INIT_FAILED;
  }

#if MIOT_ENABLE_JS
  struct v7 *v7 = s_v7 = init_v7(&arg);
#endif

#if MIOT_ENABLE_JS
  miot_prompt_init(v7, get_cfg()->debug.stdout_uart);
#endif
  return CC3200_INIT_OK;
}

void mongoose_poll_cb(void *arg);

void main_task(void *arg) {
  struct miot_event e;
  osi_MsgQCreate(&s_main_queue, "main", sizeof(e), 32 /* len */);

  enum cc3200_init_result r = cc3200_init(NULL);
  bool success = (r == CC3200_INIT_OK);
  if (!success) LOG(LL_ERROR, ("Init failed: %d", r));

#if MIOT_ENABLE_UPDATER
  miot_upd_boot_finish((r == CC3200_INIT_OK),
                       (g_boot_cfg.flags & BOOT_F_FIRST_BOOT));
#endif

  if (!success) {
    /* Arbitrary delay to make potential reboot loop less tight. */
    miot_usleep(500000);
    miot_system_restart(0);
  }

  while (1) {
    mongoose_poll(0);
    while (osi_MsgQRead(&s_main_queue, &e, V7_POLL_LENGTH_MS) == OSI_OK) {
      e.cb(e.arg);
    }
  }
}

bool miot_invoke_cb(miot_cb_t cb, void *arg) {
  struct miot_event e = {.cb = cb, .arg = arg};
  return (osi_MsgQWrite(&s_main_queue, &e, OSI_NO_WAIT) == OSI_OK);
}
