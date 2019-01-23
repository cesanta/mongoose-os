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

#include "mgos.h"
#include "mgos_system.h"

bool ubuntu_set_boottime(void);

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

// Capabilities
bool ubuntu_cap_init(void);
bool ubuntu_cap_chroot(const char *path);
