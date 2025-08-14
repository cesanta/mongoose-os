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

#pragma once

#include <functional>

#include "mgos_timers.h"

namespace mgos {

// Creates or takes ownership of an existing timer and clears it on destruction.
class Timer {
 public:
  typedef std::function<void()> Handler;

  explicit Timer(const Handler &handler);
  Timer(int msecs, int flags, const Handler &handler);
  Timer(Timer &&other);
  Timer(const Timer &other) = delete;
  ~Timer();

  void Clear();

  bool Reset(int msecs, int flags);

  bool IsValid() const;

  int GetMsecsLeft() const;

 protected:
  static void HandlerCB(void *arg);

  Handler handler_;
  mgos_timer_id id_ = MGOS_INVALID_TIMER_ID;
  bool one_shot_ = false;
};

}  // namespace mgos
