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

#pragma once

#include <string>

#include "common/platform.h"
#include "common/util/error_codes.h"

namespace mgos {

// A Status is a combination of an error code and a string message (for non-OK
// error codes).
class Status {
 public:
  // Creates an OK status
  Status();

  // Make a Status from the specified error and message.
  Status(int error, const ::std::string &error_message);

  Status(const Status &other);
  Status &operator=(const Status &other);

  // Some shortcuts
  static Status OK();  // Identical to 0-arg constructor
  static Status CANCELLED();
  static Status UNIMPLEMENTED();
  static Status UNKNOWN();

  // Accessors
  bool ok() const {
    return code_ == STATUS_OK;
  }
  int error_code() const {
    return code_;
  }
  const ::std::string &error_message() const {
    return message_;
  }

  bool operator==(const Status &x) const;
  bool operator!=(const Status &x) const;

  ::std::string ToString() const;

 private:
  int code_;
  ::std::string message_;
};

inline bool Status::operator==(const Status &other) const {
  return (this->code_ == other.code_) && (this->message_ == other.message_);
}

inline bool Status::operator!=(const Status &other) const {
  return !(*this == other);
}

Status Errorf(int code, const char *msg_fmt, ...) PRINTF_LIKE(2, 3);

Status Annotatef(const Status &other, const char *msg_fmt, ...)
    PRINTF_LIKE(2, 3);

}  // namespace mgos
