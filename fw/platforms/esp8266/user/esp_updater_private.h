/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_UPDATER_PRIVATE_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_UPDATER_PRIVATE_H_

#include <stdint.h>
#include <stdlib.h>

#include <os_type.h>

#include "common/mbuf.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"
#include "fw/platforms/esp8266/user/esp_fs.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

enum update_status {
  US_INITED,
  US_WAITING_MANIFEST_HEADER,
  US_WAITING_MANIFEST,
  US_WAITING_FILE_HEADER,
  US_WAITING_FILE,
  US_SKIPPING_DATA,
  US_SKIPPING_DESCRIPTOR,
  US_FINISHED
};

struct part_info {
  uint32_t addr;
  char sha1sum[40];
  char file_name[50];
  uint32_t real_size;
};

struct zip_file_info {
  char file_name[50];
  uint32_t file_size;
  uint32_t crc;
  uint32_t crc_current;
  uint32_t file_received_bytes;
  int has_descriptor;
};

struct update_context {
  const char *data;
  size_t data_len;
  struct mbuf unprocessed;
  struct zip_file_info file_info;
  enum update_status update_status;
  const char *status_msg;

  struct part_info fw_part;
  struct part_info fs_part;
  struct part_info *current_part;
  uint32_t current_write_address;
  uint32_t erased_till;

  char version[14];

  int parts_written;

  int slot_to_write;
  int need_reboot;

  int result;
  int archive_size;
};

extern struct update_context *s_ctx;
extern struct clubby_event *s_clubby_reply;
extern int s_clubby_upd_status;

struct update_context *updater_context_create();
void updater_context_release(struct update_context *ctx);

rboot_config *get_rboot_config();
int updater_process(struct update_context *ctx, const char *data, size_t len);
void schedule_reboot();
int is_update_finished(struct update_context *ctx);
int is_reboot_requred(struct update_context *ctx);
int is_update_in_progress();
void updater_set_status(struct update_context *ctx, enum update_status st);

struct clubby_event *load_clubby_reply(spiffs *fs);
#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_UPDATER_PRIVATE_H_ */
