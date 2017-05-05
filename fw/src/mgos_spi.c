/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_spi.h"

#if MGOS_ENABLE_SPI

static struct mgos_spi *s_global_spi;

enum mgos_init_result mgos_spi_init(void) {
  const struct sys_config_spi *cfg = &get_cfg()->spi;
  if (!cfg->enable) return MGOS_INIT_OK;
  s_global_spi = mgos_spi_create(cfg);
  return (s_global_spi != NULL ? MGOS_INIT_OK : MGOS_INIT_SPI_FAILED);
}

struct mgos_spi *mgos_spi_get_global(void) {
  return s_global_spi;
}

#endif /* MGOS_ENABLE_SPI */
