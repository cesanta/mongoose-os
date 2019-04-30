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

#include "mgos_boot_dbg.h"

#include <stdarg.h>

void mgos_boot_dbg_puts(const char *s) {
  while (*s != '\0') mgos_boot_dbg_putc(*s++);
}

void mgos_boot_dbg_putl(const char *s) {
  mgos_boot_dbg_puts(s);
  mgos_boot_dbg_putc('\r');
  mgos_boot_dbg_putc('\n');
}

void mgos_boot_dbg_printf(const char *fmt, ...) {
  va_list ap;
  char buf[300];
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  va_end(ap);
  buf[sizeof(buf) - 1] = '\0';
  mgos_boot_dbg_puts(buf);
}

void mgos_boot_dbg_putc(char c) WEAK;
void mgos_boot_dbg_putc(char c) {
  putc(c, stderr);
}
