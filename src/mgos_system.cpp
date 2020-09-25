/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
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

#include "mgos_system.hpp"

namespace mgos {

static void InvokeCBCB(void *arg) {
  auto *cb = static_cast<std::function<void()> *>(arg);
  if (!cb) return;
  (*cb)();
  delete cb;
}

bool InvokeCB(const std::function<void()> &cb, bool from_isr) {
  auto *cb_copy = new std::function<void()>(cb);
  if (cb_copy == nullptr) return false;
  if (!mgos_invoke_cb(InvokeCBCB, cb_copy, from_isr)) {
    delete cb_copy;
    return false;
  }
  return true;
}

}  // namespace mgos
