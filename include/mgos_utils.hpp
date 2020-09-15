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

#include <cstdlib>

namespace mgos {

// Like std::unique_ptr, but for pointers allocated with malloc()
// (deleting malloced pointers is not allowed).
class ScopedCPtr {
 public:
  explicit ScopedCPtr(void *ptr) : ptr_(ptr) {
  }
  ScopedCPtr(ScopedCPtr &&other) : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }
  ~ScopedCPtr() {
    reset(nullptr);
  }

  void *get() const {
    return ptr_;
  }

  void reset(void *ptr) {
    if (ptr_ != nullptr) free(ptr_);
    ptr_ = ptr;
  }

  void *release() {
    void *ptr = ptr_;
    ptr_ = nullptr;
    return ptr;
  }

 private:
  void *ptr_ = nullptr;

  ScopedCPtr(const ScopedCPtr &other) = delete;
};

}  // namespace mgos
