#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <stdio.h>

#ifndef DISABLE_OTA

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "smartjs/platforms/esp8266/user/esp_updater.h"
#include "rboot/rboot/appcode/rboot-api.h"
#include "smartjs/src/sj_config.h"
#include "smartjs/src/device_config.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_clubby.h"
#include "smartjs/src/sj_v7_ext.h"
#include "v7/v7.h"
#include "smartjs/platforms/esp8266/user/esp_fs.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Must be provided externally, usually auto-generated. */
extern const char *build_id;
extern const char *build_timestamp;
extern const char *build_version;

static int s_current_received;
static size_t s_file_size;
static int s_current_write_address;
static size_t s_written;
static int s_area_prepared;

static struct v7 *s_v7;
static v7_val_t s_updater_notify_cb;
static enum update_status s_update_status;
static os_timer_t s_update_timer;
static os_timer_t s_reboot_timer;
static char *s_manifest;
static char *s_manifest_url;
static char *s_fw_url;
static char *s_fs_url;
static char *s_fw_checksum;
static char *s_fs_checksum;
static char *s_new_fw_version;
static int s_new_rom_number;
static uint32_t s_last_received_time;
static struct mg_connection *s_current_connection;
enum download_status { DS_NOT_STARTED, DS_IN_PROGRESS, DS_COMPLETED, DS_ERROR };
static enum download_status s_download_status = DS_NOT_STARTED;

static struct clubby_event *s_clubby_reply;
static int s_clubby_upd_status;

#define UPDATER_TEMP_FILE_NAME "ota_reply.conf"

/* Forward declarations */
void set_boot_params(uint8_t new_rom, uint8_t prev_rom);
static void mg_ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static int notify_js(enum update_status us, const char *info);

rboot_config *get_rboot_config() {
  static rboot_config *cfg = NULL;
  if (cfg == NULL) {
    cfg = malloc(sizeof(*cfg));
    *cfg = rboot_get_config();
  }

  return cfg;
}

uint8_t get_current_rom() {
  return get_rboot_config()->current_rom;
}

uint32_t get_fw_addr(uint8_t rom) {
  return get_rboot_config()->roms[rom];
}

uint32_t get_fs_addr(uint8_t rom) {
  return get_rboot_config()->fs_addresses[rom];
}

uint32_t get_fs_size(uint8_t rom) {
  return get_rboot_config()->fs_sizes[rom];
}

/*
 * Compose full url from manifest full url and FW relative url
 *
 * http://myserver/myfolder/manifest.json + ....
 * /updates/blah/0x11000.bin -> myserver/updates/blah/0x11000.bin
 * updates/blah/0x11000.bin ->
 *                      myserver/myfolder/updates/blah/0x11000.bin
 * //otherserver/updates/blah/0x11000.bin ->
 *                otherserver/myfolder/updates/blah/0x11000.bin
 */
char *get_full_url(const char *base_url, const char *relative_url) {
  char *ret;
  /*
   * some of libc implementation marks asprintf with WARN_UNUSED_RESULT
   * this is fake attention to asprintf retval
   */
  int res;
  (void) res;
  if (strncmp(relative_url, "//", 2) == 0) {
    /* uri is full url, just skipping `//` */
    char *proto = NULL;
    int proto_len = 0;
    if (strncmp(base_url, "https", 5) == 0) {
      proto = "https://";
      proto_len = 8;
    } else {
      proto = "http://";
      proto_len = 7;
    }
    res = asprintf(&ret, "%.*s%s", proto_len, proto, relative_url + 2);
  } else if (strncmp(relative_url, "/", 1) == 0) {
    /* /updates/blah/0x11000.bin -> myserver/updates/blah/0x11000.bin */
    const char *start_pos = strstr(base_url, "//");
    if (start_pos == NULL) {
      start_pos = base_url;
    } else {
      start_pos += 2;
    }
    while (*start_pos != 0 && *start_pos != '/') {
      start_pos++;
    }
    if (start_pos != 0) {
      res = asprintf(&ret, "%.*s%s", (int) (start_pos - base_url), base_url,
                     relative_url);
    }
  } else {
    /*
     * updates/blah/0x11000.bin ->
     *                      myserver/myfolder/updates/blah/0x11000.bin
     */
    const char *server_base = base_url + strlen(base_url) - 1;
    while (server_base != base_url && *server_base != '/') {
      server_base--;
    }
    if (server_base != base_url) {
      res = asprintf(&ret, "%.*s%s", (int) (server_base - base_url + 1),
                     base_url, relative_url);
    }
  }

  return ret;
}

static void disconnect() {
  if (s_current_connection != NULL) {
    s_current_connection->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
  s_current_connection = NULL;
}

static void do_http_connect(const char *url) {
  LOG(LL_DEBUG, ("Url: %s", url));
  disconnect();

  s_current_connection =
      mg_connect_http(&sj_mgr, mg_ev_handler, url, NULL, NULL);
#ifdef SSL_KRYPTON
  if (memcmp(url, "https", 5) == 0 && get_cfg()->tls.enabled) {
    char *ca_file = get_cfg()->tls.ca_file[0] ? get_cfg()->tls.ca_file : NULL;
    char *server_name = get_cfg()->tls.server_name;
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
}

static void do_http_connect_by_uri(const char *base_url,
                                   const char *relative_url) {
  char *url = get_full_url(base_url, relative_url);
  do_http_connect(url);
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
      (uint32_t)(get_cfg()->update.server_timeout * 1000000)) {
    disconnect();
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

static int verify_checksum(uint32_t addr, size_t len,
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
      if (nc == s_current_connection) {
        s_current_connection = NULL;
      }
      break;
    }

    case MG_EV_HTTP_REPLY: {
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      struct http_message *hm = (struct http_message *) ev_data;
      free(s_manifest);
      s_manifest = calloc(1, hm->body.len + 1);
      memcpy(s_manifest, hm->body.p, hm->body.len);
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
            if (hm.body.len == (size_t) ~0) {
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
  (void) arg;
  switch (s_update_status) {
    case US_NOT_STARTED:
    case US_INITED: {
      /* Starting with loading manifest */
      LOG(LL_DEBUG, ("Loading manifest"));
      do_http_connect(s_manifest_url);
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
      struct json_token *toks = parse_json2(s_manifest, strlen(s_manifest));
      if (toks == NULL) {
        LOG(LL_DEBUG, ("Cannot parse manifest (len=%d)\n%s", strlen(s_manifest),
                       s_manifest));
        goto error;
      }

      if (!sj_conf_get_str(toks, "version", &s_new_fw_version)) {
        goto error;
      }

      LOG(LL_INFO, ("Version %s is available, current version %s",
                    s_new_fw_version, build_version));
      /*
       * build_version & s_new_fw_version is a date in
       * %Y%m%d%H%M%S format, so, we can lex compare them
       */
      if (strcmp(s_new_fw_version, build_version) > 0) {
        LOG(LL_INFO, ("Starting update to %s", s_new_fw_version));
      } else {
        LOG(LL_INFO, ("Nothing to do, exiting update"));
        set_update_status(US_NOTHING_TODO);
        goto cleanup;
      }

      s_new_rom_number = get_current_rom() == 1 ? 0 : 1;
      LOG(LL_DEBUG, ("ROM to write: %d", s_new_rom_number));

      if (!sj_conf_get_str(toks, "parts.fw.src", &s_fw_url)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FW url: %s", s_fw_url));

      if (!sj_conf_get_str(toks, "parts.fs.src", &s_fs_url)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FS url: %s", s_fs_url));

      if (!sj_conf_get_str(toks, "parts.fw.cs_sha1", &s_fw_checksum)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FW checksum: %s", s_fw_checksum));

      if (!sj_conf_get_str(toks, "parts.fs.cs_sha1", &s_fs_checksum)) {
        goto error;
      }
      LOG(LL_DEBUG, ("FS checksum: %s", s_fs_checksum));

      s_file_size = s_current_received = s_area_prepared = 0;
      s_current_write_address = get_fw_addr(s_new_rom_number);
      LOG(LL_DEBUG, ("Address to write ROM: %X", s_current_write_address));

      LOG(LL_INFO, ("Loading %s", s_fw_url));
      do_http_connect_by_uri(s_manifest_url, s_fw_url);
      set_update_status(US_DOWNLOADING_FW);
      set_download_status(DS_IN_PROGRESS);
      goto cleanup;

    error:
      LOG(LL_ERROR, ("Invalid manifest file"));
      set_update_status(US_ERROR);

    cleanup:
      free(toks);
      break;
    }

    case US_DOWNLOADING_FW: {
      if (s_download_status == DS_IN_PROGRESS) {
        verify_timeout();
      } else if (s_download_status == DS_COMPLETED) {
        if (verify_checksum(get_fw_addr(s_new_rom_number), s_file_size,
                            s_fw_checksum) != 0) {
          set_update_status(US_ERROR);
        } else {
          /* Start FS download */
          LOG(LL_DEBUG, ("Loading %s", s_fs_url));
          s_file_size = s_current_received = s_area_prepared = 0;
          s_current_write_address = get_fs_addr(s_new_rom_number);
          do_http_connect_by_uri(s_manifest_url, s_fs_url);
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
        if (verify_checksum(get_fs_addr(s_new_rom_number), s_file_size,
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
      disconnect();

      int len;
      char *upd_data = sj_clubby_repl_to_bytes(s_clubby_reply, &len);
      FILE *tmp_file = fopen(UPDATER_TEMP_FILE_NAME, "w");
      if (tmp_file == NULL || upd_data == NULL) {
        /* There is nothing we can do */
        LOG(LL_ERROR, ("Cannot save update status"));
      } else {
        fwrite(upd_data, 1, len, tmp_file);
        fclose(tmp_file);
      }

      LOG(LL_INFO,
          ("Version %s downloaded, restarting system", s_new_fw_version));
      os_timer_disarm(&s_update_timer);

      set_boot_params(s_new_rom_number, rboot_get_current_rom());

      if (!notify_js(US_COMPLETED, NULL)) {
        schedule_reboot();
      }

      set_update_status(US_NOT_STARTED);
      break;
    }

    case US_NOTHING_TODO: {
      disconnect();
      os_timer_disarm(&s_update_timer);
      /* Leave this status to support UI stuff */
      notify_js(US_NOTHING_TODO, NULL);
      sj_clubby_send_reply(s_clubby_reply, 0, "Already updated");
      break;
    }

    case US_ERROR: {
      disconnect();
      LOG(LL_ERROR, ("Update failed"));
      os_timer_disarm(&s_update_timer);
      notify_js(US_ERROR, NULL);
      sj_clubby_send_reply(s_clubby_reply, 1, "Update failed");
    }
  }
}

enum update_status update_get_status(void) {
  return s_update_status;
}

int is_in_progress() {
  int ret = (s_update_status != US_NOT_STARTED &&
             s_update_status != US_NOTHING_TODO && s_update_status != US_ERROR);
  LOG(LL_DEBUG, ("In progress: %d", ret));
  return ret;
}

void update_start(const char *manifest_url) {
  if (is_in_progress()) {
    LOG(LL_INFO, ("Update already in progress"));
    return;
  }

  free(s_manifest_url);

  if (manifest_url != NULL) {
    s_manifest_url = strdup(manifest_url);
  } else {
    /* Fallback to cfg-stored manifest url. Debug only */
    s_manifest_url = strdup(get_cfg()->update.manifest_url);
  }

  set_update_status(US_INITED);
  s_last_received_time = system_get_time();
  os_timer_disarm(&s_update_timer);
  os_timer_setfn(&s_update_timer, update_timer_cb, NULL);
  os_timer_arm(&s_update_timer, 1000, 1);
}

void set_boot_params(uint8_t new_rom, uint8_t prev_rom) {
  rboot_config rc = rboot_get_config();
  rc.previous_rom = prev_rom;
  rc.current_rom = new_rom;
  /*
   * using two flags - is_first_boot & fw_updated
   * we need them to manage fw fallbacks
   */
  rc.is_first_boot = 1;
  rc.fw_updated = 1;
  rc.boot_attempts = 0;
  rboot_set_config(&rc);
}

static int file_copy(spiffs *old_fs, char *file_name) {
  LOG(LL_DEBUG, ("Copying %s", file_name));
  int ret = 0;
  FILE *f = NULL;
  spiffs_stat stat;

  spiffs_file fd = SPIFFS_open(old_fs, file_name, SPIFFS_RDONLY, 0);
  if (fd < 0) {
    int err = SPIFFS_errno(old_fs);
    if (err == SPIFFS_ERR_NOT_FOUND) {
      LOG(LL_WARN, ("File %s not found, skipping", file_name));
      return 1;
    } else {
      LOG(LL_ERROR, ("Failed to open %s, error %d", file_name, err));
      return 0;
    }
  }

  if (SPIFFS_fstat(old_fs, fd, &stat) != SPIFFS_OK) {
    LOG(LL_ERROR, ("Update failed: cannot get previous %s size (%d)", file_name,
                   SPIFFS_errno(old_fs)));
    goto exit;
  }

  LOG(LL_DEBUG, ("Previous %s size is %d", file_name, stat.size));

  f = fopen(file_name, "w");
  if (f == NULL) {
    LOG(LL_ERROR, ("Failed to open %s", file_name));
    goto exit;
  }

  char buf[512];
  int32_t readen, to_read = 0, total = 0;
  to_read = MIN(sizeof(buf), stat.size);

  while (to_read != 0) {
    if ((readen = SPIFFS_read(old_fs, fd, buf, to_read)) < 0) {
      LOG(LL_ERROR, ("Failed to read %d bytes from %s, error %d", to_read,
                     file_name, SPIFFS_errno(old_fs)));
      goto exit;
    }

    if (fwrite(buf, 1, readen, f) != (size_t) readen) {
      LOG(LL_ERROR, ("Failed to write %d bytes to %s", readen, file_name));
      goto exit;
    }

    total += readen;
    LOG(LL_DEBUG, ("Read: %d, remains: %d", readen, stat.size - total));

    to_read = MIN(sizeof(buf), (stat.size - total));
  }

  LOG(LL_DEBUG, ("Wrote %d to %s", total, file_name));

  ret = 1;

exit:
  if (fd >= 0) {
    SPIFFS_close(old_fs, fd);
  }

  if (f != NULL) {
    fclose(f);
  }

  return ret;
}

static struct clubby_event *load_clubby_reply(spiffs *fs) {
  struct clubby_event *ret = NULL;
  spiffs_stat st;
  char *reply_str = NULL;
  spiffs_file upd_file = -1;

  upd_file = SPIFFS_open(fs, UPDATER_TEMP_FILE_NAME, SPIFFS_RDONLY, 0);
  if (upd_file < 0) {
    LOG(LL_ERROR, ("Cannot open updater file"));
    goto cleanup;
  }

  if (SPIFFS_fstat(fs, upd_file, &st) != SPIFFS_OK) {
    LOG(LL_ERROR, ("Cannot get size of updater"));
    goto cleanup;
  }

  LOG(LL_ERROR, ("Updater file size = %d", st.size));
  reply_str = malloc(st.size);
  if (reply_str == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    goto cleanup;
  }

  if ((uint32_t) SPIFFS_read(fs, upd_file, reply_str, st.size) != st.size) {
    LOG(LL_ERROR, ("Cannot read data from updater file"));
    goto cleanup;
  }

  ret = sj_clubby_bytes_to_repl(reply_str, st.size);
  if (ret == NULL) {
    LOG(LL_ERROR, ("Cannot create clubby reply"));
    goto cleanup;
  }

cleanup:
  free(reply_str);

  if (upd_file >= 0) {
    SPIFFS_close(fs, upd_file);
  }

  return ret;
}

static int load_data_from_old_fs(uint32_t old_fs_addr) {
  uint8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
  uint8_t spiffs_fds[32 * 2];
  spiffs old_fs;
  int ret = 0;
  spiffs_DIR dir, *dir_ptr = NULL;
  LOG(LL_DEBUG, ("Mounting fs %d @ 0x%x", FS_SIZE, old_fs_addr));
  if (fs_mount(&old_fs, old_fs_addr, FS_SIZE, spiffs_work_buf, spiffs_fds,
               sizeof(spiffs_fds))) {
    LOG(LL_ERROR, ("Update failed: cannot mount previous file system"));
    return 1;
  }
  /*
   * here we can use fread & co to read
   * current fs and SPIFFs functions to read
   * old one
   */

  /* Copy predefined overrides files */
  if (!file_copy(&old_fs, OVERRIDES_JSON_FILE)) {
    goto cleanup;
  }

  /* Copy files with name started with FILE_UPDATE_PREF ("imp_" by default) */
  dir_ptr = SPIFFS_opendir(&old_fs, ".", &dir);
  if (dir_ptr == NULL) {
    LOG(LL_ERROR, ("Failed to open root directory"));
    goto cleanup;
  }

  struct spiffs_dirent de, *de_ptr;
  while ((de_ptr = SPIFFS_readdir(dir_ptr, &de)) != NULL) {
    if (memcmp(de_ptr->name, FILE_UPDATE_PREF, strlen(FILE_UPDATE_PREF)) == 0) {
      if (!file_copy(&old_fs, (char *) de_ptr->name)) {
        goto cleanup;
      }
    }
  }

  ret = 1;

  s_clubby_reply = load_clubby_reply(&old_fs);
/* Do not rollback fw if load_clubby_reply failed */

cleanup:
  if (dir_ptr != NULL) {
    SPIFFS_closedir(dir_ptr);
  }

  SPIFFS_unmount(&old_fs);

  return ret;
}

static void reboot_timer_cb(void *arg) {
  (void) arg;
  /*
   * just reboot system, fw boot is not commited so, rboot will do actual
   * rollback
   */
  system_restart();
}

int finish_update() {
  if (!get_rboot_config()->fw_updated) {
    if (get_rboot_config()->is_first_boot != 0) {
      LOG(LL_INFO, ("Firmware was rolled back, commiting it"));
      get_rboot_config()->is_first_boot = 0;
      rboot_set_config(get_rboot_config());
      s_clubby_upd_status = 1; /* Once we connect wifi we send status 1 */
    }
    return 1;
  }

  /*
   * We merge FS _after_ booting to new FW, to give a chance
   * to change merge behavior in new FW
   */
  if (load_data_from_old_fs(get_fs_addr(get_rboot_config()->previous_rom)) !=
      0) {
    /* Ok, commiting update */
    get_rboot_config()->is_first_boot = 0;
    get_rboot_config()->fw_updated = 0;
    rboot_set_config(get_rboot_config());
    LOG(LL_DEBUG, ("Firmware commited"));

    return 1;
  } else {
    LOG(LL_ERROR,
        ("Failed to merge filesystem, rollback to previous firmware"));

    schedule_reboot();
    return 0;
  }

  return 1;
}

void schedule_reboot() {
  /*
   * For unknown reason, calling system_restart() from user_init/system_init_cb
   * couses nothing but core dump. So, arming timer for reboot
   * TODO(alashkin): figure out why so
   */
  os_timer_setfn(&s_reboot_timer, reboot_timer_cb, NULL);
  os_timer_arm(&s_reboot_timer, 1000, 0);
}

static int notify_js(enum update_status us, const char *info) {
  if (!v7_is_undefined(s_updater_notify_cb)) {
    if (info == NULL) {
      sj_invoke_cb1(s_v7, s_updater_notify_cb, v7_mk_number(us));
    } else {
      sj_invoke_cb2(s_v7, s_updater_notify_cb, v7_mk_number(us),
                    v7_mk_string(s_v7, info, ~0, 1));
    };

    return 1;
  }

  return 0;
}

static void handle_clubby_ready(struct clubby_event *evt, void *user_data) {
  (void) user_data;
  LOG(LL_DEBUG, ("Clubby is ready"));

  if (s_clubby_reply == NULL && s_clubby_upd_status != 0) {
    /*
     * If we are here, FW was rolled back and we have to pickup
     * updater info from current FS
     */
    s_clubby_reply = load_clubby_reply(get_fs());
    remove(UPDATER_TEMP_FILE_NAME);
  }

  if (s_clubby_reply) {
    LOG(LL_DEBUG, ("Found reply to send"));
    s_clubby_reply->context = evt->context;
    sj_clubby_send_reply(
        s_clubby_reply, s_clubby_upd_status,
        s_clubby_upd_status == 0 ? "Updated successfully" : "Update failed");
    sj_clubby_free_reply(s_clubby_reply);
    s_clubby_reply = NULL;
  };
}

static void handle_clubby_update(struct clubby_event *evt, void *user_data) {
  (void) user_data;
  LOG(LL_DEBUG, ("Command received: %.*s", evt->request.cmd_body->len,
                 evt->request.cmd_body->ptr));

  struct json_token *args = find_json_token(evt->request.cmd_body, "args");
  if (args == NULL || args->type != JSON_TYPE_OBJECT) {
    goto bad_request;
  }

  struct json_token *section = find_json_token(args, "section");
  struct json_token *blob_url = find_json_token(args, "blob_url");

  /*
   * TODO(alashkin): enable update for another files, not
   * firmware only
   */
  if (section == NULL || section->type != JSON_TYPE_STRING ||
      strncmp(section->ptr, "firmware", section->len) != 0 ||
      blob_url == NULL || blob_url->type != JSON_TYPE_STRING) {
    goto bad_request;
  }

  LOG(LL_DEBUG, ("manifest url: %.*s", blob_url->len, blob_url->ptr));

  sj_clubby_free_reply(s_clubby_reply);
  s_clubby_reply = sj_clubby_create_reply(evt);

  /*
   * If user setup callback for updater, just call it.
   * User can start update with Sys.updater.start()
   */
  if (!is_in_progress()) {
    char *manifest_url = calloc(1, blob_url->len + 1);
    if (manifest_url == NULL) {
      LOG(LL_ERROR, ("Out of memory"));
      return;
    }

    memcpy(manifest_url, blob_url->ptr, blob_url->len);

    if (!notify_js(US_NOT_STARTED, manifest_url)) {
      update_start(manifest_url);
    }

    free(manifest_url);
  }
  return;

bad_request:
  sj_clubby_send_reply(evt, 1, "malformed request");
}

static enum v7_err Updater_startupdate(struct v7 *v7, v7_val_t *res) {
  LOG(LL_DEBUG, ("Starting update"));
  v7_val_t manifest_url_v = v7_arg(v7, 0);
  if (v7_is_undefined(manifest_url_v)) {
    /* Using default manifest address */
    update_start(NULL);
  } else if (v7_is_string(manifest_url_v)) {
    update_start(v7_to_cstring(v7, &manifest_url_v));
  } else {
    printf("Invalid arguments\n");
    *res = v7_mk_boolean(0);
    return V7_OK;
  }

  *res = v7_mk_boolean(1);
  return V7_OK;
}

/*
 * Example of notification function:
 * function upd(ev, url) {
 *      if (ev == Sys.updater.GOT_REQUEST) {
 *         print("Starting update from", url);
 *         Sys.updater.start(url);
 *       }  else if(ev == Sys.updater.NOTHING_TODO) {
 *         print("No need to update");
 *       } else if(ev == Sys.updater.FAILED) {
 *         print("Update failed");
 *       } else if(ev == Sys.updater.COMPLETED) {
 *         print("Update completed");
 *         Sys.reboot();
 *       }
 * }
 */

static enum v7_err Updater_notify(struct v7 *v7, v7_val_t *res) {
  v7_val_t cb = v7_arg(v7, 0);
  if (!v7_is_callable(v7, cb)) {
    printf("Invalid arguments\n");
    *res = v7_mk_boolean(0);
    return V7_OK;
  }

  if (!v7_is_undefined(s_updater_notify_cb)) {
    v7_disown(v7, &s_updater_notify_cb);
  }

  s_updater_notify_cb = cb;
  v7_own(v7, &s_updater_notify_cb);

  *res = v7_mk_boolean(1);
  return V7_OK;
}

static void handle_update_req(struct mg_connection *c, int ev, void *p) {
  (void) c;
  (void) ev;
  (void) p;
  LOG(LL_DEBUG, ("Incoming update request"));
}

void init_updater(struct v7 *v7) {
  s_v7 = v7;
  v7_val_t updater = v7_mk_object(v7);
  v7_val_t sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);
  s_updater_notify_cb = v7_mk_undefined();

  v7_def(v7, sys, "updater", ~0, V7_DESC_ENUMERABLE(0), updater);
  v7_set_method(v7, updater, "start", Updater_startupdate);
  v7_set_method(v7, updater, "notify", Updater_notify);

  v7_def(s_v7, updater, "GOT_REQUEST", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(US_NOT_STARTED));

  v7_def(s_v7, updater, "COMPLETED", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(US_COMPLETED));

  v7_def(s_v7, updater, "NOTHING_TODO", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(US_NOTHING_TODO));

  v7_def(s_v7, updater, "FAILED", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(US_ERROR));

  sj_clubby_register_global_command("/v1/SWUpdate.Update", handle_clubby_update,
                                    NULL);

  sj_clubby_register_global_command(clubby_cmd_ready, handle_clubby_ready,
                                    NULL);

  device_register_http_endpoint("/update", handle_update_req);
}

#endif /* DISABLE_OTA */
