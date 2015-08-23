#include <malloc.h>
#include <string.h>

#include "hw_types.h"
#include "prcm.h"
#include "rom_map.h"

#include "simplelink.h"
#include "device.h"

#include "cc3200_sj_hal.h"
#include "sj_hal.h"
#include "sj_v7_ext.h"
#include "v7.h"

#include "oslib/osi.h"

extern OsiMsgQ_t s_prompt_input_q;

void sj_exec_with(struct v7 *v7, const char *code, v7_val_t this_obj) {
  struct prompt_event pe;
  struct v7_exec_event_data *ped = calloc(1, sizeof(*ped));
  ped->code = strdup(code);
  ped->this_obj = this_obj;
  v7_own(v7, &ped->this_obj);
  pe.type = V7_EXEC_EVENT;
  pe.data = ped;
  osi_MsgQWrite(&s_prompt_input_q, &pe, OSI_WAIT_FOREVER);
}

/* Defined in linker script. */
extern unsigned long _heap;
extern unsigned long _eheap;

size_t sj_get_free_heap_size() {
  size_t avail = ((char *) &_eheap - (char *) &_heap);
  struct mallinfo mi = mallinfo();
  avail -= mi.arena;    /* Claimed by allocator. */
  avail += mi.fordblks; /* Free in the area claimed by allocator. */
  return avail;
}

size_t sj_get_fs_memory_usage() {
  return 0; /* Not even sure if it's possible to tell. */
}

void sj_wdt_feed() {
  /* TODO */
}

void sj_system_restart() {
  /* Turns out to be not that easy. In particular, using *Reset functions is
   * not a good idea.
   * https://e2e.ti.com/support/wireless_connectivity/f/968/p/424736/1516404
   * Instead, the recommended way is to enter hibernation with immediate wakeup.
   */
  sl_Stop(50 /* ms */);
  MAP_PRCMHibernateIntervalSet(328 /* 32KHz ticks, 100 ms */);
  MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
  MAP_PRCMHibernateEnter();
}

void sj_set_timeout(int msecs, v7_val_t *cb) {
  /* TODO */
}

void sj_usleep(int usecs) {
  osi_Sleep(usecs / 1000 /* ms */);
}
