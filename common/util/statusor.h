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

#ifndef CS_COMMON_UTIL_STATUSOR_H_
#define CS_COMMON_UTIL_STATUSOR_H_

#include <assert.h>

#include <common/util/status.h>

namespace util {

// A StatusOr holds a Status (in the case of an error), or a value T.
template <typename T>
class StatusOr {
 public:
  // Has status UNKNOWN.
  inline StatusOr();

  // Builds from a non-OK status. Crashes if an OK status is specified.
  inline StatusOr(const Status& status);

  // Builds from the specified value.
  inline StatusOr(const T& value);
  inline StatusOr(T&& value);

  // Copy and move constructors.
  inline StatusOr(const StatusOr& other) = default;
  inline StatusOr(StatusOr&& other);

  // Conversion copy constructor, T must be copy constructible from U.
  template<typename U> inline StatusOr(const StatusOr<U>& other);
  template<typename U> inline StatusOr(StatusOr<U>&& other);

  // Assignment operator, copy and move varieties.
  inline const StatusOr& operator=(const StatusOr& other);
  inline StatusOr& operator=(StatusOr&& other);

  // Conversion assignment operator, T must be assignable from U
  template<typename U>
  inline const StatusOr& operator=(const StatusOr<U>& other);
  template<typename U>
  inline StatusOr& operator=(StatusOr<U>&& other);

  // Accessors.
  const Status& status() const { return status_; }

  // Shorthand for status().ok().
  bool ok() const { return status_.ok(); }

  // Returns value or crashes if ok() is false.
  inline const T& ValueOrDie() const;

  // Extracts the value, reverts the object to UNKNOWN status.
  inline T MoveValueOrDie();

  template<typename U> friend class StatusOr;

 private:
  Status status_;
  T value_;
};

// Implementation.

template<typename T>
inline StatusOr<T>::StatusOr()
    : status_(error::UNKNOWN, "") {}

template<typename T>
inline StatusOr<T>::StatusOr(const Status& status)
    : status_(status) {
  assert(!status.ok());
}

template<typename T>
inline StatusOr<T>::StatusOr(const T& value)
    : value_(value) {
}

template<typename T>
inline StatusOr<T>::StatusOr(T&& value)
    : value_(std::move(value)) {
}

template<typename T>
inline StatusOr<T>::StatusOr(StatusOr<T>&& other)
    : status_(other.status_), value_(std::move(other.value_)) {
  other.status_ = Status::UNKNOWN;
}

template<typename T>
template<typename U>
inline StatusOr<T>::StatusOr(const StatusOr<U>& other)
    : status_(other.status_), value_(other.value_) {
}

template<typename T>
template<typename U>
inline StatusOr<T>::StatusOr(StatusOr<U>&& other)
    : status_(other.status_), value_(std::move(other.value_)) {
  other.status_ = Status::UNKNOWN;
}

template <typename T>
inline const StatusOr<T>& StatusOr<T>::operator=(const StatusOr& other) {
  status_ = other.status_;
  if (status_.ok()) value_ = other.value_;
  return *this;
}

template<typename T>
inline StatusOr<T>& StatusOr<T>::operator=(StatusOr&& other) {
  status_ = std::move(other.status_);
  if (status_.ok()) value_ = std::move(other.value_);
  other.status_ = Status::UNKNOWN;
  return *this;
}

template<typename T>
template<typename U>
inline const StatusOr<T>& StatusOr<T>::operator=(const StatusOr<U>& other) {
  status_ = other.status_;
  if (status_.ok()) value_ = other.value_;
  return *this;
}

template<typename T>
template<typename U>
inline StatusOr<T>& StatusOr<T>::operator=(StatusOr<U>&& other) {
  status_ = other.status_;
  if (status_.ok()) value_ = std::move(other.value_);
  other.status_ = Status::UNKNOWN;
  return *this;
}

template<typename T>
const T& StatusOr<T>::ValueOrDie() const {
  assert(ok());
  return value_;
}

template<typename T>
inline T StatusOr<T>::MoveValueOrDie() {
  assert(ok());
  status_ = Status::UNKNOWN;
  return std::move(value_);
}

}  // namespace util

#endif /* CS_COMMON_UTIL_STATUSOR_H_ */
