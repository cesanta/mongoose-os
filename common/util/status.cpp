// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/util/status.h"

#include <stdarg.h>

#include "common/str_util.h"

namespace mgos {

// static
Status Status::OK() {
  return Status();
}

// static
Status Status::CANCELLED() {
  return Status(STATUS_CANCELLED, "Cancelled");
}

// static
Status Status::UNIMPLEMENTED() {
  return Status(STATUS_UNIMPLEMENTED, "Unimplemented");
}

// static
Status Status::UNKNOWN() {
  return Status(STATUS_UNKNOWN, "Unknown");
}

Status::Status() : code_(STATUS_OK), message_("") {
}

Status::Status(int error, const std::string &error_message)
    : code_(error), message_(error_message) {
  if (code_ == STATUS_OK) {
    message_.clear();
  }
}

Status::Status(const Status &other)
    : code_(other.code_), message_(other.message_) {
}

Status &Status::operator=(const Status &other) {
  code_ = other.code_;
  message_ = other.message_;
  return *this;
}

std::string Status::ToString() const {
  if (message_.empty()) return StatusToString(code_);
  return StatusToString(code_) + ": " + message_;
}

Status Errorf(int code, const char *msg_fmt, ...) {
  va_list ap;
  va_start(ap, msg_fmt);
  char *msg = NULL;
  mg_avprintf(&msg, 0, msg_fmt, ap);
  va_end(ap);
  Status res(code, std::string(msg ? msg : ""));
  free(msg);
  return res;
}

Status Annotatef(const Status &other, const char *msg_fmt, ...) {
  va_list ap;
  va_start(ap, msg_fmt);
  char *msg = NULL;
  mg_avprintf(&msg, 0, msg_fmt, ap);
  va_end(ap);
  Status res = Errorf(other.error_code(), "%s : %s", (msg ? msg : ""),
                      other.error_message().c_str());
  free(msg);
  return res;
}

}  // namespace mgos
