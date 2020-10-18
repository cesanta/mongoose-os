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

#include "mgos_utils.hpp"

#include "mgos.h"

namespace mgos {

std::string SPrintf(const char *fmt, ...) {
  char buf[100];
  va_list ap;
  va_start(ap, fmt);
  char *out = buf;
  int n = mg_avprintf(&out, sizeof(buf), fmt, ap);
  va_end(ap);
  std::string res(out, n);
  if (out != buf) free(out);
  return res;
}

ScopedCPtr::ScopedCPtr(void *ptr) : ptr_(ptr) {
}

ScopedCPtr::ScopedCPtr(ScopedCPtr &&other) : ptr_(other.ptr_) {
  other.ptr_ = nullptr;
}

ScopedCPtr::~ScopedCPtr() {
  reset(nullptr);
}

void *ScopedCPtr::get() const {
  return ptr_;
}

void ScopedCPtr::reset(void *ptr) {
  if (ptr_ != nullptr) free(ptr_);
  ptr_ = ptr;
}

void *ScopedCPtr::release() {
  void *ptr = ptr_;
  ptr_ = nullptr;
  return ptr;
}

}  // namespace mgos
