/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/cs_dbg.h"
#include "mgos_system.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ubuntu_flags {
  uid_t uid;
  gid_t gid;
  char *chroot;
  int secure;
};

// Logging for the main process (using different colors)
int logm_print_prefix(enum cs_log_level l, const char *func, const char *file);
void logm_printf(const char *fmt, ...);

#define LOGM(l, x)                                  \
  do {                                              \
    if (logm_print_prefix(l, __func__, __FILE__)) { \
      logm_printf x;                                \
    }                                               \
    fprintf(stderr, "\033[0m\r\n");                 \
  } while (0)

// Mongoose helper initializers
bool ubuntu_set_boottime(void);
bool ubuntu_set_nsleep100(void);

// Create a socketpair
bool ubuntu_ipc_init(void);

// Initialize the socketpair in main
bool ubuntu_ipc_init_main(void);

// Initialize the socketpair in mongoose
bool ubuntu_ipc_init_mongoose(void);

// Handle incoming IPC requests from mongoose to main.
bool ubuntu_ipc_handle(uint16_t timeout_ms);

// Destroy the socketpair in main
bool ubuntu_ipc_destroy_main(void);

// Destroy the socketpair in mongoose
bool ubuntu_ipc_destroy_mongoose(void);

// The Ubuntu side of the watchdog
bool ubuntu_wdt_ok(void);
bool ubuntu_wdt_feed(void);
bool ubuntu_wdt_enable(void);
bool ubuntu_wdt_disable(void);
void ubuntu_wdt_set_timeout(int secs);

// Capabilities (drop privs, chroot, et al)
bool ubuntu_cap_init(void);

// Flags -- returns true if flags are parsed correctly.
// Returns false (and prints usage to stdout) otherwise.
bool ubuntu_flags_init(int argc, char **argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */
