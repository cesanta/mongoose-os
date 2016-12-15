/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_atca.h"

#if MIOT_ENABLE_ATCA

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_dbg.h"

#include "fw/src/miot_i2c.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"

#include "cryptoauthlib.h"
#include "hal/atca_hal.h"
#include "host/atca_host.h"

/*
 * This is a HAL implementation for the Atmel/Microchip CryptoAuthLib.
 * It translates ATCA interface calls into MIOT API calls.
 * Currently only I2C is implemented.
 */

static ATCA_STATUS miot_atca_hal_i2c_init(void *hal, ATCAIfaceCfg *cfg) {
  (void) hal;
  (void) cfg;
  return ATCA_SUCCESS;
}

static ATCA_STATUS miot_atca_hal_i2c_post_init(ATCAIface iface) {
  (void) iface;
  return ATCA_SUCCESS;
}

static ATCA_STATUS miot_atca_hal_i2c_send(ATCAIface iface, uint8_t *txdata,
                                          int txlength) {
  ATCAIfaceCfg *cfg = atgetifacecfg(iface);
  struct miot_i2c *i2c = (struct miot_i2c *) atgetifacehaldat(iface);

  ATCA_STATUS status = ATCA_TX_TIMEOUT;

  if (txlength > 0) {
    txdata[0] = 0x03; /* Word Address Value = Command */
  } else {
    /* This is our (MIOT) extension. In this mode WAV is set by wake or idle. */
  }
  txlength++; /* Include Word Address value in txlength */

  if (miot_i2c_start(i2c, (cfg->atcai2c.slave_address >> 1), I2C_WRITE) ==
      I2C_ACK) {
    /* Successful transmission ends with an ACK. */
    if (miot_i2c_send_bytes(i2c, txdata, txlength) == I2C_ACK) {
      status = ATCA_SUCCESS;
    } else {
      status = ATCA_TX_FAIL;
    }
    miot_i2c_stop(i2c);
  }
  return status;
}

static ATCA_STATUS miot_atca_hal_i2c_receive(ATCAIface iface, uint8_t *rxdata,
                                             uint16_t *rxlength) {
  ATCAIfaceCfg *cfg = atgetifacecfg(iface);
  struct miot_i2c *i2c = (struct miot_i2c *) atgetifacehaldat(iface);

  ATCA_STATUS status = ATCA_RX_TIMEOUT;

  int retries = cfg->rx_retries;

  while (retries-- > 0 && status != ATCA_SUCCESS) {
    uint8_t count;
    if (miot_i2c_start(i2c, (cfg->atcai2c.slave_address >> 1), I2C_READ) !=
        I2C_ACK) {
      continue;
    }
    count = rxdata[0] = miot_i2c_read_byte(i2c, I2C_ACK);
    if ((count < ATCA_RSP_SIZE_MIN) || (count > *rxlength)) {
      miot_i2c_stop(i2c);
      status = ATCA_INVALID_SIZE;
      break;
    }
    miot_i2c_read_bytes(i2c, count - 1, rxdata + 1, I2C_NAK);
    miot_i2c_stop(i2c);
    status = ATCA_SUCCESS;
  }
  /*
   * rxlength is a pointer, which suggests taht the actual number of bytes
   * received should be returned in the value pointed to, but none of the
   * existing HAL implementations do it.
   */
  return status;
}

static ATCA_STATUS miot_atca_hal_i2c_wake(ATCAIface iface) {
  ATCAIfaceCfg *cfg = atgetifacecfg(iface);
  struct miot_i2c *i2c = (struct miot_i2c *) atgetifacehaldat(iface);

  ATCA_STATUS status = ATCA_WAKE_FAILED;

  uint8_t response[4] = {0x00, 0x00, 0x00, 0x00};
  uint8_t expected_response[4] = {0x04, 0x11, 0x33, 0x43};

  /*
   * ATCA devices define "wake up" token as START, 80 us of SDA low, STOP.
   * Simulate this by trying to write to address 0.
   * We're not expecting an ACK, so don't check for return value.
   */

  miot_i2c_start(i2c, 0, I2C_WRITE);
  miot_i2c_stop(i2c);

  /* After wake signal we need to wait some time for device to init. */
  atca_delay_us(cfg->wake_delay);

  /* Receive the wake response. */
  uint16_t len = sizeof(response);
  status = miot_atca_hal_i2c_receive(iface, response, &len);
  if (status == ATCA_SUCCESS) {
    if (memcmp(response, expected_response, 4) != 0) {
      status = ATCA_WAKE_FAILED;
    }
  }

  return status;
}

ATCA_STATUS miot_atca_hal_i2c_idle(ATCAIface iface) {
  uint8_t idle_cmd = 0x02;
  return miot_atca_hal_i2c_send(iface, &idle_cmd, 0);
}

ATCA_STATUS miot_atca_hal_i2c_sleep(ATCAIface iface) {
  uint8_t sleep_cmd = 0x01;
  return miot_atca_hal_i2c_send(iface, &sleep_cmd, 0);
}

ATCA_STATUS miot_atca_hal_i2c_release(void *hal_data) {
  (void) hal_data;
  return ATCA_SUCCESS;
}

ATCA_STATUS hal_iface_init(ATCAIfaceCfg *cfg, ATCAHAL_t *hal) {
  if (cfg->iface_type != ATCA_I2C_IFACE) return ATCA_BAD_PARAM;
  struct miot_i2c *i2c = miot_i2c_get_global();
  if (i2c == NULL) return ATCA_GEN_FAIL;
  hal->halinit = &miot_atca_hal_i2c_init;
  hal->halpostinit = &miot_atca_hal_i2c_post_init;
  hal->halsend = &miot_atca_hal_i2c_send;
  hal->halreceive = &miot_atca_hal_i2c_receive;
  hal->halwake = &miot_atca_hal_i2c_wake;
  hal->halidle = &miot_atca_hal_i2c_idle;
  hal->halsleep = &miot_atca_hal_i2c_sleep;
  hal->halrelease = &miot_atca_hal_i2c_release;
  hal->hal_data = i2c;
  return ATCA_SUCCESS;
}

ATCA_STATUS hal_iface_release(ATCAIfaceType ifacetype, void *hal_data) {
  if (ifacetype != ATCA_I2C_IFACE) return ATCA_BAD_PARAM;
  /* I2C connection is global, do not free. */
  (void) hal_data;
  return ATCA_SUCCESS;
}

bool s_atca_is_available = false;

/* Invoked from mbedTLS during ECDH phase of the handshake. */
int mbedtls_atca_is_available() {
  return s_atca_is_available;
}

uint8_t mbedtls_atca_get_ecdh_slots_mask() {
  return get_cfg()->sys.atca.ecdh_slots_mask;
}

enum miot_init_result miot_atca_init(void) {
  uint32_t revision;
  uint32_t
      serial[(ATCA_SERIAL_NUM_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
  bool config_is_locked, data_is_locked;
  ATCA_STATUS status;
  struct sys_config_sys_atca *acfg = &get_cfg()->sys.atca;
  ATCAIfaceCfg *atca_cfg;

  if (!(acfg->enable || get_cfg()->sys.atca_enable)) {
    return MIOT_INIT_OK;
  }

  uint8_t addr = acfg->i2c_addr;
  /*
   * It's a bit unfortunate that Atmel requires address already shifted by 1.
   * If user specifies address > 0x80, it must be already shifted since I2C bus
   * addresses > 0x7f are invalid.
   */
  if (addr < 0x7f) addr <<= 1;
  atca_cfg = &cfg_ateccx08a_i2c_default;
  if (atca_cfg->atcai2c.slave_address != addr) {
    atca_cfg = (ATCAIfaceCfg *) calloc(1, sizeof(*atca_cfg));
    memcpy(atca_cfg, &cfg_ateccx08a_i2c_default, sizeof(*atca_cfg));
    atca_cfg->atcai2c.slave_address = addr;
  }

  status = atcab_init(atca_cfg);
  if (status != ATCA_SUCCESS) {
    LOG(LL_ERROR, ("ATECC508 init failed"));
    goto out;
  }

  status = atcab_info((uint8_t *) &revision);
  if (status != ATCA_SUCCESS) {
    LOG(LL_ERROR, ("Failed to get info"));
    goto out;
  }

  status = atcab_read_serial_number((uint8_t *) serial);
  if (status != ATCA_SUCCESS) {
    LOG(LL_ERROR, ("Failed to get serial number"));
    goto out;
  }

  status = atcab_is_locked(LOCK_ZONE_CONFIG, &config_is_locked);
  status = atcab_is_locked(LOCK_ZONE_DATA, &data_is_locked);
  if (status != ATCA_SUCCESS) {
    LOG(LL_ERROR, ("Failed to get data lock status"));
    goto out;
  }

  LOG(LL_INFO,
      ("ATECC508 @ 0x%02x: rev 0x%04x S/N 0x%04x%04x%02x, zone "
       "lock status: %s, %s; ECDH slots: 0x%02x",
       addr >> 1, htonl(revision), htonl(serial[0]), htonl(serial[1]),
       *((uint8_t *) &serial[2]), (config_is_locked ? "yes" : "no"),
       (data_is_locked ? "yes" : "no"), mbedtls_atca_get_ecdh_slots_mask()));

  s_atca_is_available = true;

out:
  /*
   * We do not free atca_cfg in case of an error even if it was allocated
   * because it is referenced by ATCA basic object.
   */
  return (status == ATCA_SUCCESS ? MIOT_INIT_OK : MIOT_INIT_ATCA_FAILED);
}

void atca_delay_ms(uint32_t delay) {
  miot_usleep(delay * 1000);
}

void atca_delay_us(uint32_t delay) {
  miot_usleep(delay);
}

#endif /* MIOT_ENABLE_ATCA */
