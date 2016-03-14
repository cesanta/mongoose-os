/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_UTIL_LOGGING_H_
#define CS_COMMON_UTIL_LOGGING_H_

#include <iostream>

namespace util {

class LogMessageAndDie;

#define CHECK(x) \
  if (!(x)) ::util::LogMessageAndDie(__FILE__, __LINE__).stream() \
      << "CHECK failed: " << #x << ": "

#define __CHECK_OP(x, y, op, ops) \
  { \
    const auto __xc = (x); \
    const auto __yc = (y); \
    if (!(__xc op __yc)) \
      ::util::LogMessageAndDie(__FILE__, __LINE__).stream() \
         << "CHECK failed: " << #x << ops << #y << " (" << __xc << " vs " << __yc << ")"; \
  }
#define CHECK_EQ(x, y) __CHECK_OP(x, y, ==, " == ")
#define CHECK_NE(x, y) __CHECK_OP(x, y, !=, " != ")
#define CHECK_GT(x, y) __CHECK_OP(x, y, >,  " > ")
#define CHECK_GE(x, y) __CHECK_OP(x, y, >=, " >= ")
#define CHECK_LT(x, y) __CHECK_OP(x, y, <,  " < ")
#define CHECK_LE(x, y) __CHECK_OP(x, y, <=, " <= ")

#define __CHECK_OP_M(x, y, m, op, ops) \
  { \
    const auto __xc = (x); \
    const auto __yc = (y); \
    if (!(__xc op __yc)) \
      ::util::LogMessageAndDie(__FILE__, __LINE__).stream() \
         << "CHECK failed: " << #x << ops << #y << " (" << __xc << " vs " << __yc << "): " << m; \
  }
#define CHECK_EQ_M(x, y, m) __CHECK_OP_M(x, y, m, ==, " == ")
#define CHECK_NE_M(x, y, m) __CHECK_OP_M(x, y, m, !=, " != ")
#define CHECK_GT_M(x, y, m) __CHECK_OP_M(x, y, m, >,  " > ")
#define CHECK_GE_M(x, y, m) __CHECK_OP_M(x, y, m, >=, " >= ")
#define CHECK_LT_M(x, y, m) __CHECK_OP_M(x, y, m, <,  " < ")
#define CHECK_LE_M(x, y, m) __CHECK_OP_M(x, y, m, <=, " <= ")

class LogMessage {
 public:
  LogMessage(const char* file, int line);
  virtual ~LogMessage();

  std::ostream& stream();

 private:
  const char* file_;
  const int line_;

  LogMessage(const LogMessage& other) = delete;
};

class LogMessageAndDie : public LogMessage {
 public:
  LogMessageAndDie(const char* file, int line);
  ~LogMessageAndDie() override;
};

}  // namespace util

#endif /* CS_COMMON_UTIL_LOGGING_H_ */
