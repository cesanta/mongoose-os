/*
 * Copyright (c) 2021 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdint.h>

// esf_buf monitor detects a WiFi TX stall that sometimes happens
// and that cannot be recovered from other than by rebooting.

#ifndef ESP_ESF_BUF_MONITOR_INTERVAL_MS
#define ESP_ESF_BUF_MONITOR_INTERVAL_MS 5000
#endif

// This callback will be invoked once stuck TX is detected.
// Default (weak) implementation is to reboot the device.
void esp_esf_buf_monitor_failure(void);

void esp_esf_buf_monitor_init(void);
