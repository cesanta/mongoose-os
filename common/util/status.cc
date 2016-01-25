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

#include <iostream>

#include <common/util/status.h>

using ::std::ostream;
using ::std::string;

namespace util {

namespace {

const Status &GetOk() {
  static const Status status;
  return status;
}

const Status &GetCancelled() {
  static const Status status(error::CANCELLED, "Cancelled");
  return status;
}

const Status &GetUnimplemented() {
  static const Status status(error::UNIMPLEMENTED, "Unimplemented");
  return status;
}

const Status &GetUnknown() {
  static const Status status(error::UNKNOWN, "Unknown");
  return status;
}

}  // namespace

Status::Status() : code_(error::OK), message_("") {}

Status::Status(error::Code error, const string& error_message)
    : code_(error), message_(error_message) {
  if (code_ == error::OK) {
    message_.clear();
  }
}

Status::Status(const Status& other)
    : code_(other.code_), message_(other.message_) {}

Status& Status::operator=(const Status& other) {
  code_ = other.code_;
  message_ = other.message_;
  return *this;
}

const Status& Status::OK = GetOk();
const Status& Status::CANCELLED = GetCancelled();
const Status& Status::UNIMPLEMENTED = GetUnimplemented();
const Status& Status::UNKNOWN = GetUnknown();

string Status::ToString() const {
  if (message_.empty()) return error::ToString(code_);
  return error::ToString(code_) + ": " + message_;
}

extern ostream& operator<<(ostream& os, const Status& status) {
  os << status.ToString();
  return os;
}

}  // namespace util
