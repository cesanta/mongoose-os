/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <iostream>

// no_extern_c_check

namespace mgos {

class LogMessageAndDie;

#define CHECK(x)                                        \
  if (!(x))                                             \
  ::util::LogMessageAndDie(__FILE__, __LINE__).stream() \
      << "CHECK failed: " << #x << ": "

#define __CHECK_OP(x, y, op, ops)                                          \
  {                                                                        \
    const auto __xc = (x);                                                 \
    const auto __yc = (y);                                                 \
    if (!(__xc op __yc))                                                   \
      ::util::LogMessageAndDie(__FILE__, __LINE__).stream()                \
          << "CHECK failed: " << #x << ops << #y << " (" << __xc << " vs " \
          << __yc << ")";                                                  \
  }
#define CHECK_EQ(x, y) __CHECK_OP(x, y, ==, " == ")
#define CHECK_NE(x, y) __CHECK_OP(x, y, !=, " != ")
#define CHECK_GT(x, y) __CHECK_OP(x, y, >, " > ")
#define CHECK_GE(x, y) __CHECK_OP(x, y, >=, " >= ")
#define CHECK_LT(x, y) __CHECK_OP(x, y, <, " < ")
#define CHECK_LE(x, y) __CHECK_OP(x, y, <=, " <= ")

#define __CHECK_OP_M(x, y, m, op, ops)                                     \
  {                                                                        \
    const auto __xc = (x);                                                 \
    const auto __yc = (y);                                                 \
    if (!(__xc op __yc))                                                   \
      ::util::LogMessageAndDie(__FILE__, __LINE__).stream()                \
          << "CHECK failed: " << #x << ops << #y << " (" << __xc << " vs " \
          << __yc << "): " << m;                                           \
  }
#define CHECK_EQ_M(x, y, m) __CHECK_OP_M(x, y, m, ==, " == ")
#define CHECK_NE_M(x, y, m) __CHECK_OP_M(x, y, m, !=, " != ")
#define CHECK_GT_M(x, y, m) __CHECK_OP_M(x, y, m, >, " > ")
#define CHECK_GE_M(x, y, m) __CHECK_OP_M(x, y, m, >=, " >= ")
#define CHECK_LT_M(x, y, m) __CHECK_OP_M(x, y, m, <, " < ")
#define CHECK_LE_M(x, y, m) __CHECK_OP_M(x, y, m, <=, " <= ")

class LogMessage {
 public:
  LogMessage(const char *file, int line);
  virtual ~LogMessage();

  std::ostream &stream();

 private:
  const char *file_;
  const int line_;

  LogMessage(const LogMessage &other) = delete;
};

class LogMessageAndDie : public LogMessage {
 public:
  LogMessageAndDie(const char *file, int line);
  ~LogMessageAndDie() override;
};

}  // namespace mgos
