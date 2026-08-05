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

#include "mgos_timers.hpp"

namespace mgos {

Timer::Timer(const Handler &handler) : handler_(handler) {
}

Timer::Timer(int msecs, int flags, const Handler &handler)
    : handler_(handler) {
  id_ = mgos_set_timer(msecs, flags, &Timer::HandlerCB, this);
  one_shot_ = (flags & MGOS_TIMER_REPEAT) == 0;
}

Timer::Timer(Timer &&other)
    : handler_(other.handler_), id_(other.id_) {
  other.id_ = MGOS_INVALID_TIMER_ID;
  other.handler_ = nullptr;
}

Timer::~Timer() {
  Clear();
}

void Timer::Clear() {
  if (id_ == MGOS_INVALID_TIMER_ID) return;
  mgos_clear_timer(id_);
  id_ = MGOS_INVALID_TIMER_ID;
}

bool Timer::Reset(int msecs, int flags) {
  Clear();
  id_ = mgos_set_timer(msecs, flags, &Timer::HandlerCB, this);
  one_shot_ = (flags & MGOS_TIMER_REPEAT) == 0;
  return IsValid();
}

bool Timer::IsValid() const {
  return (id_ != MGOS_INVALID_TIMER_ID);
}

int Timer::GetMsecsLeft() const {
  if (!IsValid()) return 0;
  struct mgos_timer_info ti;
  if (mgos_get_timer_info(id_, &ti)) {
    return ti.msecs_left;
  }
  return 0;
}

// static
void Timer::HandlerCB(void *arg) {
  auto *st = static_cast<Timer *>(arg);
  if (st->one_shot_) {
    st->id_ = MGOS_INVALID_TIMER_ID;
  }
  if (st->handler_) {
    st->handler_();
  }
}

}  // namespace mgos
