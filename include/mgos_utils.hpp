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
#include <string>

namespace mgos {

std::string SPrintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Like std::unique_ptr, but for pointers allocated with malloc()
// (deleting malloced pointers is not allowed).
class ScopedCPtr {
 public:
  explicit ScopedCPtr(void *ptr);
  ScopedCPtr(ScopedCPtr &&other);
  ScopedCPtr(const ScopedCPtr &other) = delete;
  ~ScopedCPtr();

  void *get() const;

  void reset(void *ptr);

  void *release();

 private:
  void *ptr_ = nullptr;
};

}  // namespace mgos
