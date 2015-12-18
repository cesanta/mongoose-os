#ifndef ESP_UPDATER_H_INCLUDED
#define ESP_UPDATER_H_INCLUDED

#ifndef DISABLE_OTA

#include "mongoose.h"

enum update_status {
  US_NOT_STARTED,
  US_WAITING_METADATA,
  US_GOT_METADATA,
  US_DOWNLOADING_FW,
  US_DOWNLOADING_FS,
  US_COMPLETED,
  US_ERROR,
  US_NOTHING_TODO
};

void update_start(struct mg_mgr *mgr);
enum update_status update_get_status(void);
int finish_update();
uint32_t get_fs_addr();
void rollback_fw();

#endif

#endif /* DISABLE_OTA */
