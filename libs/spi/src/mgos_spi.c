/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_spi.h"

#include "mgos_sys_config.h"

static struct mgos_spi *s_global_spi;

bool mgos_spi_init(void) {
  if (!mgos_sys_config_get_spi_enable()) return true;
  s_global_spi = mgos_spi_create(mgos_sys_config_get_spi());
  return (s_global_spi != NULL);
}

struct mgos_spi *mgos_spi_get_global(void) {
  return s_global_spi;
}

bool mgos_spi_config_from_json(const struct mg_str cfg_json,
                               struct mgos_config_spi *cfg) {
  return mgos_sys_config_parse_sub(cfg_json, "spi", cfg);
}
