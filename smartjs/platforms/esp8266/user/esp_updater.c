#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <stdio.h>

#ifndef DISABLE_OTA

#include "esp_updater.h"
#include "esp_missing_includes.h"
#include "rboot/rboot/appcode/rboot-api.h"
#include "smartjs/src/sj_config.h"
#include "smartjs/src/device_config.h"
#include "smartjs/src/sj_mongoose.h"

/*
 * It looks too dangerous to put this numbers to
 * metadata file, so, they are compile-time consts
 */
static unsigned int fw_addresses[] = {FW1_ADDR, FW2_ADDR};
static unsigned int fs_addresses[] = {FW1_FS_ADDR, FW2_FS_ADDR};

static int s_current_received;
static int s_file_size;
static int s_current_write_address;
static int s_written;
static int s_area_prepared;

static enum update_status s_update_status;
static os_timer_t s_update_timer;
static os_timer_t s_reboot_timer;
static char *s_metadata;
static char *s_fw_url;
static char *s_fs_url;
static char *s_fw_checksum;
static char *s_fs_checksum;
static char *s_new_fw_timestamp;
static int s_new_rom_number;
static uint32_t s_last_received_time;
static struct mg_connection *s_current_connection;
enum download_status { DS_NOT_STARTED, DS_IN_PROGRESS, DS_COMPLETED, DS_ERROR };
static enum download_status s_download_status = DS_NOT_STARTED;

/* Forward declarations */
void set_boot_params(uint8_t new_rom, uint8_t prev_rom);
static void mg_ev_handler(struct mg_connection *nc, int ev, void *ev_data);

static void do_http_connect(struct mg_mgr *mgr, const char *uri) {
  static char *url;
  asprintf(&url, "http%s://%s%s", get_cfg()->update.tls_ena ? "s" : "",
           get_cfg()->update.server_address, uri);
  LOG(LL_DEBUG, ("Full url: %s", url));
  s_current_connection = mg_connect_http(mgr, mg_ev_handler, url, NULL, NULL);
#ifdef ESP_SSL_KRYPTON
  if (get_cfg()->update.tls_ena) {
    char *ca_file =
        get_cfg()->update.tls_ca_file[0] ? get_cfg()->update.tls_ca_file : NULL;
    char *server_name = get_cfg()->update.tls_server_name;
    mg_set_ssl(s_current_connection, NULL, ca_file);
    if (server_name[0] == '\0') {
      char *p;
      server_name = strdup(get_cfg()->update.server_address);
      p = strchr(server_name, ':');
      if (p != NULL) *p = '\0';
    }
    SSL_CTX_kr_set_verify_name(s_current_connection->ssl_ctx, server_name);
    if (server_name != get_cfg()->update.tls_server_name) free(server_name);
  }
#endif
  free(url);
}

static void set_download_status(enum download_status ds) {
  LOG(LL_DEBUG,
      ("Download status: %d -> %d", (int) s_download_status, (int) ds));
  s_download_status = ds;
}

static void set_update_status(enum update_status us) {
  LOG(LL_DEBUG, ("Update status: %d -> %d", (int) s_update_status, us));
  s_update_status = us;
}

static int verify_timeout() {
  if (system_get_time() - s_last_received_time >
      get_cfg()->update.server_timeout * 1000000) {
    if (s_current_connection != NULL) {
      s_current_connection->flags |= MG_F_CLOSE_IMMEDIATELY;
    }
    set_update_status(US_ERROR);
    set_download_status(DS_ERROR);
    LOG(LL_ERROR, ("Timeout"));
    return 1;
  } else {
    return 0;
  }
}

static int prepare_area(uint32_t addr, int size) {
  int sector_no = addr / 0x1000;

  while (size >= 0) {
    if (spi_flash_erase_sector(sector_no) != 0) {
      LOG(LL_ERROR, ("Cannot erase sector %X", sector_no));
      return -1;
    }

    pp_soft_wdt_restart();

    size -= 0x1000;
    sector_no++;
  }

  return 0;
}

static void bin2hex(const uint8_t *src, int src_len, char *dst) {
  int i = 0;
  for (i = 0; i < src_len; i++) {
    sprintf(dst, "%02x", (int) *src);
    dst += 2;
    src += 1;
  }
}

static int verify_checksum(uint32_t addr, int len,
                           const char *provided_checksum) {
  uint8_t read_buf[4 * 100];
  char written_checksum[50];
  int to_read;

  LOG(LL_DEBUG, ("Verifying checksum"));
  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);

  while (len != 0) {
    if (len > sizeof(read_buf)) {
      to_read = sizeof(read_buf);
    } else {
      to_read = len;
    }

    if (spi_flash_read(addr, (uint32_t *) read_buf, to_read) != 0) {
      LOG(LL_ERROR, ("Failed to read %d bytes from %X", to_read, addr));
      return -1;
    }

    cs_sha1_update(&ctx, read_buf, to_read);
    addr += to_read;
    len -= to_read;

    pp_soft_wdt_restart();
  }

  cs_sha1_final(read_buf, &ctx);
  bin2hex(read_buf, 20, written_checksum);
  LOG(LL_DEBUG, ("Written FW checksum: %s Provided checksum: %s",
                 written_checksum, provided_checksum));

  if (strcmp(written_checksum, provided_checksum) != 0) {
    LOG(LL_ERROR, ("Checksum verification failed"));
    return 1;
  } else {
    LOG(LL_INFO, ("Checksum verification ok"));
    return 0;
  }
}

static void download_error(struct mg_connection *nc) {
  set_download_status(DS_ERROR);
  nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}

static void mg_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_CONNECT: {
      if (*(int *) ev_data != 0) {
        LOG(LL_ERROR, ("fw_updater: connect() failed: %d", (int *) ev_data));
        download_error(nc);
      } else {
        LOG(LL_DEBUG, ("fw_updater: connected"));
        if (s_update_status != US_WAITING_METADATA) {
          /* Disable HTTP proto handler, we'll process response ourselves. */
          nc->proto_handler = NULL;
        }
      }
      break;
    }

    case MG_EV_CLOSE: {
      s_current_connection = NULL;
      break;
    }

    case MG_EV_HTTP_REPLY: {
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      struct http_message *hm = (struct http_message *) ev_data;
      free(s_metadata);
      s_metadata = calloc(1, hm->body.len + 1);
      memcpy(s_metadata, hm->body.p, hm->body.len);
      LOG(LL_DEBUG, ("Metadata size: %d", (int) hm->body.len));
      set_download_status(DS_COMPLETED);
      break;
    }

    case MG_EV_RECV: {
      s_last_received_time = system_get_time();
      if (s_update_status == US_WAITING_METADATA) {
        /* Metadata is processed in MS_EV_HTTP_REPLY */
        break;
      }
      struct http_message hm;
      struct mbuf *io = &nc->recv_mbuf;
      int parsed;
      s_current_received += (int) nc->recv_mbuf.len;
      LOG(LL_DEBUG, ("fw_updater: received %d bytes, %d bytes total",
                     (int) nc->recv_mbuf.len, s_current_received));

      if (s_file_size == 0) {
        memset(&hm, 0, sizeof(hm));
        /* Waiting for Content-Length */
        if ((parsed = mg_parse_http(io->buf, io->len, &hm, 0)) > 0) {
          if (hm.body.len != 0) {
            LOG(LL_DEBUG, ("fw_updater: file size: %d", (int) hm.body.len));
            if (hm.body.len == ~0) {
              LOG(LL_DEBUG,
                  ("Invalid content-length, perhaps chunked-encoding"));
              download_error(nc);
              break;
            } else {
              s_file_size = hm.body.len;
            }
            mbuf_remove(io, parsed);
          }
        }
      }

      if (s_file_size != 0) {
        if (!s_area_prepared) {
          /*
           * Possibly, it is better to erase on demand,
           * sector-by-sector. On another hand, erasing of 500Kb takes
           * ~1-2 sec but simplifies code
           */
          LOG(LL_DEBUG, ("Erasing flash"));
          if (prepare_area(s_current_write_address, s_file_size) != 0) {
            download_error(nc);
            return;
          }
          LOG(LL_DEBUG, ("Flash ready"));
          s_area_prepared = 1;
          s_written = 0;
        }

        /* Write operation must be 4-bytes aligned */
        int to_write = io->len & -4;

        fprintf(stderr, ".");

        LOG(LL_DEBUG, ("Writing %d @ 0x%X", to_write,
                       s_current_write_address + s_written));
        if (spi_flash_write(s_current_write_address + s_written,
                            (uint32_t *) io->buf, to_write) != 0) {
          LOG(LL_ERROR, ("Cannot write to address %X",
                         s_current_write_address + s_written));
          download_error(nc);
          return;
        }

        mbuf_remove(io, to_write);
        s_written += to_write;
        int rest = s_file_size - s_written;
        LOG(LL_DEBUG, ("Write done, written %d of %d (%d to go)", s_written,
                       s_file_size, rest));

        if (rest != 0 && rest < 4) {
          /* Filesize % 4 != 0 and we write last chunk */
          uint8_t align_buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
          LOG(LL_DEBUG, ("Writing end of file (%d dytes) to %X", rest,
                         s_current_write_address + s_written));
          memcpy(align_buf, io->buf, rest);
          if (spi_flash_write(s_current_write_address + s_written,
                              (uint32_t *) align_buf, 4) != 0) {
            LOG(LL_ERROR, ("Cannot write to address %X",
                           s_current_write_address + s_written));
            download_error(nc);
            return;
          }

          mbuf_remove(io, rest);
          s_written += rest;
        }
      }

      if (s_written == s_file_size) {
        LOG(LL_INFO, ("New firmware written (%d bytes)", s_written));
        set_download_status(DS_COMPLETED);
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      }

      /* We'll see these bytes again next time. */
      s_current_received -= io->len;

      break;
    }
  }
}

void update_timer_cb(void *arg) {
  switch (s_update_status) {
    case US_NOT_STARTED: {
      /* Starting with loading metadata */
      LOG(LL_DEBUG,
          ("Loading metadata from %s", get_cfg()->update.metadata_url));
      do_http_connect((struct mg_mgr *) arg, get_cfg()->update.metadata_url);
      set_update_status(US_WAITING_METADATA);
      set_download_status(DS_IN_PROGRESS);
      break;
    }

    case US_WAITING_METADATA: {
      if (s_download_status == DS_IN_PROGRESS) {
        verify_timeout();
      } else if (s_download_status == DS_COMPLETED) {
        set_update_status(US_GOT_METADATA);
      } else {
        set_update_status(US_ERROR);
      }
      break;
    }

    case US_GOT_METADATA: {
      struct json_token *toks = parse_json2(s_metadata, strlen(s_metadata));
      if (toks == NULL) {
        LOG(LL_DEBUG, ("Cannot parse metadata (len=%d)\n%s", strlen(s_metadata),
                       s_metadata));
        goto error;
      }

      if (!sj_conf_get_str(toks, "timestamp", &s_new_fw_timestamp)) {
        goto error;
      }

      LOG(LL_INFO, ("Version %s is available, current version %s",
                    s_new_fw_timestamp, FW_TIMESTAMP));
      /*
       * FW_TIMESTAMP & s_new_fw_timestamp is a date in
       * %Y%m%d%H%M format, so, we can lex compare them
       */
      if (strcmp(s_new_fw_timestamp, FW_TIMESTAMP) > 0) {
        LOG(LL_INFO, ("Starting update to %s", s_new_fw_timestamp));
      } else {
        LOG(LL_INFO, ("Nothing to do, exiting update"));
        set_update_status(US_NOTHING_TODO);
        break;
      }

      s_new_rom_number = rboot_get_current_rom() == 1 ? 0 : 1;
      LOG(LL_DEBUG, ("ROM to write: %d", s_new_rom_number));

      if (!sj_conf_get_str(toks, "fw_url", &s_fw_url)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FW url: %s", s_fw_url));

      if (!sj_conf_get_str(toks, "fs_url", &s_fs_url)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FS url: %s", s_fs_url));

      if (!sj_conf_get_str(toks, "fw_checksum", &s_fw_checksum)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FW checksum: %s", s_fw_checksum));

      if (!sj_conf_get_str(toks, "fs_checksum", &s_fs_checksum)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FS checksum: %s", s_fs_checksum));

      s_file_size = s_current_received = s_area_prepared = 0;
      s_current_write_address = fw_addresses[s_new_rom_number];
      LOG(LL_DEBUG, ("Address to write ROM: %X", s_current_write_address));

      LOG(LL_INFO, ("Loading %s", s_fw_url));
      do_http_connect((struct mg_mgr *) arg, s_fw_url);
      set_update_status(US_DOWNLOADING_FW);
      set_download_status(DS_IN_PROGRESS);
      goto cleanup;

    error:
      LOG(LL_ERROR, ("Invalid metadata file"));
      set_update_status(US_ERROR);

    cleanup:
      free(toks);
      break;
    }

    case US_DOWNLOADING_FW: {
      if (s_download_status == DS_IN_PROGRESS) {
        verify_timeout();
      } else if (s_download_status == DS_COMPLETED) {
        if (verify_checksum(fw_addresses[s_new_rom_number], s_file_size,
                            s_fw_checksum) != 0) {
          set_update_status(US_ERROR);
        } else {
          /* Start FS download */
          LOG(LL_DEBUG, ("Loading %s", s_fs_url));
          s_file_size = s_current_received = s_area_prepared = 0;
          s_current_write_address = fs_addresses[s_new_rom_number];
          do_http_connect((struct mg_mgr *) arg, s_fs_url);
          set_update_status(US_DOWNLOADING_FS);
          set_download_status(DS_IN_PROGRESS);
        }
      } else {
        set_update_status(DS_ERROR);
      }
    }

    case US_DOWNLOADING_FS: {
      if (s_download_status == DS_IN_PROGRESS) {
        verify_timeout();
      } else if (s_download_status == DS_COMPLETED) {
        if (verify_checksum(fs_addresses[s_new_rom_number], s_file_size,
                            s_fs_checksum) != 0) {
          set_update_status(US_ERROR);
        } else {
          set_update_status(US_COMPLETED);
        }

      } else {
        set_update_status(US_ERROR);
      }
      break;
    }

    case US_COMPLETED: {
      LOG(LL_INFO,
          ("Version %s downloaded, restarting system", s_new_fw_timestamp));
      os_timer_disarm(&s_update_timer);

      set_boot_params(s_new_rom_number, rboot_get_current_rom());

      system_restart();
      set_update_status(US_NOT_STARTED); /* Paranoia? */
      break;
    }

    case US_NOTHING_TODO: {
      os_timer_disarm(&s_update_timer);
      /* Leave this status to support UI stuff */
      break;
    }

    case US_ERROR: {
      LOG(LL_ERROR, ("Update failed"));
      os_timer_disarm(&s_update_timer);
    }
  }
}

enum update_status update_get_status(void) {
  return s_update_status;
}

void update_start(struct mg_mgr *mgr) {
  if (s_update_status != US_NOT_STARTED && s_update_status != US_NOTHING_TODO &&
      s_update_status != US_ERROR) {
    LOG(LL_INFO, ("Update already in progress"));
    return;
  }

  s_update_status = US_NOT_STARTED;
  s_last_received_time = system_get_time();
  os_timer_setfn(&s_update_timer, update_timer_cb, mgr);
  os_timer_arm(&s_update_timer, 1000, 1);
}

void set_boot_params(uint8_t new_rom, uint8_t prev_rom) {
  rboot_config rc = rboot_get_config();
  rc.previous_rom = prev_rom;
  rc.current_rom = new_rom;
  /*
   * using tow flags - is_first_boot & fw_updated
   * we need them to manage fw fallbacks
   */
  rc.is_first_boot = 1;
  rc.fw_updated = 1;
  rc.boot_attempts = 0;
  rboot_set_config(&rc);
}

static void reboot_timer_cb(void *arg) {
  /*
   * just reboot system, fw boot is not commited so, rboot will do actual
   * rollback
   */
  system_restart();
}

int finish_update() {
  rboot_config rc = rboot_get_config();
  if (!rc.fw_updated) {
    if (rc.is_first_boot != 0) {
      LOG(LL_INFO, ("Firmware was rolled back, commiting it"));
      rc.is_first_boot = 0;
      rboot_set_config(&rc);
    }
    return 0;
  }

  /* Ok, commiting update */
  rc.is_first_boot = 0;
  rc.fw_updated = 0;
  rboot_set_config(&rc);
  LOG(LL_DEBUG, ("Firmware commited"));

  return 1;
}

uint32_t get_fs_addr() {
  rboot_config rc = rboot_get_config();
  return fs_addresses[rc.current_rom];
}

void rollback_fw() {
  /*
   * For unknown reason, calling system_restart() from user_init/system_init_cb
   * couses nothing but core dump. So, arming timer for reboot
   * TODO(alashkin): figure out why so
   */
  os_timer_setfn(&s_reboot_timer, reboot_timer_cb, NULL);
  os_timer_arm(&s_reboot_timer, 1000, 0);
}

static v7_val_t Updater_startupdate(struct v7 *v7) {
  LOG(LL_DEBUG, ("Starting update"));
  update_start(&sj_mgr);

  return v7_create_boolean(1);
}

void init_updater(struct v7 *v7) {
  v7_val_t updater = v7_create_object(v7);
  v7_val_t sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);

  v7_set(v7, sys, "updater", ~0, 0, updater);
  v7_set_method(v7, updater, "start", Updater_startupdate);
}

#endif /* DISABLE_OTA */
