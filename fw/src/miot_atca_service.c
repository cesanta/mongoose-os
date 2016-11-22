/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_atca.h"

#if MIOT_ENABLE_RPC && MIOT_ENABLE_ATCA && MIOT_ENABLE_ATCA_SERVICE

#include "common/cs_crc32.h"

#include "fw/src/miot_rpc.h"

#include "cryptoauthlib.h"

static void miot_atca_get_config(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    goto clean;
  }

  uint8_t config[ATCA_CONFIG_SIZE];
  for (int i = 0; i < ATCA_CONFIG_SIZE / ATCA_BLOCK_SIZE; i++) {
    int offset = (i * ATCA_BLOCK_SIZE);
    ATCA_STATUS status =
        atcab_read_zone(ATCA_ZONE_CONFIG, 0 /* slot */, i /* block */,
                        0 /* offset */, config + offset, ATCA_BLOCK_SIZE);
    if (status != ATCA_SUCCESS) {
      mg_rpc_send_errorf(ri, 500, "Failed to read config zone block %d: 0x%02x",
                         i, status);
      ri = NULL;
      goto clean;
    }
  }

  uint32_t crc32 = cs_crc32(0, config, sizeof(config));
  mg_rpc_send_responsef(ri, "{config: %V, crc32: %u}", config, sizeof(config),
                        crc32);
  ri = NULL;

clean:
  (void) cb_arg;
  (void) args;
}

static void miot_atca_set_config(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  uint8_t *config = NULL;
  uint32_t config_len = 0;
  uint32_t crc32 = 0;
  json_scanf(args.p, args.len, "{config: %V, crc32: %u}", &config, &config_len,
             &crc32);

  if (config_len != ATCA_CONFIG_SIZE) {
    mg_rpc_send_errorf(ri, 400, "Expected %d bytes, got %d",
                       (int) ATCA_CONFIG_SIZE, (int) config_len);
    ri = NULL;
    goto clean;
  }

  uint32_t got_crc32 = cs_crc32(0, config, config_len);
  if (got_crc32 != crc32) {
    mg_rpc_send_errorf(ri, 400, "CRC mismatch (expected %u, got %u)", crc32,
                       got_crc32);
    ri = NULL;
    goto clean;
  }

  ATCA_STATUS status = atcab_write_config_zone(ATECC508A, config);
  if (status != ATCA_SUCCESS) {
    mg_rpc_send_errorf(ri, 500, "Failed to set config: 0x%02x", status);
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

clean:
  if (config != NULL) free(config);
  (void) cb_arg;
}

static void miot_atca_lock_zone(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  int zone = -1;
  json_scanf(args.p, args.len, "{zone: %d}", &zone);

  ATCA_STATUS status;
  uint8_t lock_response = 0;
  switch (zone) {
    case LOCK_ZONE_CONFIG:
      status = atcab_lock_config_zone(&lock_response);
      break;
    case LOCK_ZONE_DATA:
      status = atcab_lock_data_zone(&lock_response);
      break;
    default:
      mg_rpc_send_errorf(ri, 403, "Invalid zone");
      ri = NULL;
      goto clean;
  }

  if (status != ATCA_SUCCESS) {
    mg_rpc_send_errorf(ri, 500, "Failed to lock zone %d: 0x%02x", zone, status);
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

clean:
  (void) cb_arg;
}

static void miot_atca_set_key(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  int slot = -1;
  uint8_t *key = NULL;
  uint32_t key_len = 0;
  uint32_t crc32 = 0;
  json_scanf(args.p, args.len, "{slot: %d, key: %V, crc32: %u}", &slot, &key,
             &key_len, &crc32);

  if (slot < 0 || slot > 15) {
    mg_rpc_send_errorf(ri, 400, "Invalid slot");
    ri = NULL;
    goto clean;
  }

  if (key_len != ATCA_KEY_SIZE) {
    mg_rpc_send_errorf(ri, 400, "Expected %d bytes, got %d",
                       (int) ATCA_KEY_SIZE, (int) key_len);
    ri = NULL;
    goto clean;
  }

  uint32_t got_crc32 = cs_crc32(0, key, key_len);
  if (got_crc32 != crc32) {
    mg_rpc_send_errorf(ri, 400, "CRC mismatch (expected %u, got %u)", crc32,
                       got_crc32);
    ri = NULL;
    goto clean;
  }

  uint8_t key_arg[4 + ATCA_KEY_SIZE];
  memset(key_arg, 0, 4);
  memcpy(key_arg + 4, key, ATCA_KEY_SIZE);
  ATCA_STATUS status = atcab_priv_write(slot, key_arg, 0, NULL);
  if (status != ATCA_SUCCESS) {
    mg_rpc_send_errorf(ri, 500, "Failed to set private key: 0x%02x", status);
    ri = NULL;
    goto clean;
  }

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

clean:
  if (key != NULL) free(key);
  (void) cb_arg;
}
static void miot_atca_get_or_gen_key(struct mg_rpc_request_info *ri,
                                     void *cb_arg, struct mg_rpc_frame_info *fi,
                                     struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  uint8_t pubkey[ATCA_PUB_KEY_SIZE];
  int slot = -1;
  json_scanf(args.p, args.len, "{slot: %d}", &slot);

  if (slot < 0 || slot > 15) {
    mg_rpc_send_errorf(ri, 400, "Invalid slot");
    ri = NULL;
    goto clean;
  }

  if (strcmp((const char *) cb_arg, "/ATCA.GenKey") == 0) {
    ATCA_STATUS status = atcab_genkey(slot, pubkey);
    if (status != ATCA_SUCCESS) {
      mg_rpc_send_errorf(ri, 500, "Failed generate key on slot %d: 0x%02x",
                         slot, status);
      ri = NULL;
      goto clean;
    }
  } else {
    ATCA_STATUS status = atcab_get_pubkey(slot, pubkey);
    if (status != ATCA_SUCCESS) {
      mg_rpc_send_errorf(ri, 500, "Failed get public key for slot %d: 0x%02x",
                         slot, status);
      ri = NULL;
      goto clean;
    }
  }

  uint32_t crc32 = cs_crc32(0, pubkey, sizeof(pubkey));
  mg_rpc_send_responsef(ri, "{pubkey: %V, crc32: %u}", pubkey, sizeof(pubkey),
                        crc32);
  ri = NULL;

clean:
  return;
}

enum miot_init_result miot_atca_service_init(void) {
  struct mg_rpc *c = miot_rpc_get_global();
  mg_rpc_add_handler(c, mg_mk_str("/ATCA.GetConfig"), miot_atca_get_config,
                     NULL);
  mg_rpc_add_handler(c, mg_mk_str("/ATCA.SetConfig"), miot_atca_set_config,
                     NULL);
  mg_rpc_add_handler(c, mg_mk_str("/ATCA.LockZone"), miot_atca_lock_zone, NULL);
  mg_rpc_add_handler(c, mg_mk_str("/ATCA.SetKey"), miot_atca_set_key, NULL);
  mg_rpc_add_handler(c, mg_mk_str("/ATCA.GenKey"), miot_atca_get_or_gen_key,
                     "/ATCA.GenKey");
  mg_rpc_add_handler(c, mg_mk_str("/ATCA.GetPubKey"), miot_atca_get_or_gen_key,
                     "/ATCA.GetPubKey");
  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_ATCA && MIOT_ENABLE_ATCA_SERVICE */
