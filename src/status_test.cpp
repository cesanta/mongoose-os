#include <common/util/status.h>

#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <common/util/logging.h>

namespace util {

using util::Status;

void VerifyOk(const Status* s) {
  EXPECT_TRUE(s->ok());
  EXPECT_EQ(error::OK, s->error_code());
  EXPECT_EQ("", s->error_message());
  EXPECT_EQ("OK", s->ToString());
}

TEST(StatusTest, DefaultConstructedOK) {
  Status s;
  VerifyOk(&s);
}

TEST(StatusTest, ConstantOK) {
  VerifyOk(&Status::OK);
}

TEST(StatusTest, ConstantCancelled) {
  EXPECT_FALSE(Status::CANCELLED.ok());
  EXPECT_EQ(error::CANCELLED, Status::CANCELLED.error_code());
  EXPECT_EQ("", Status::CANCELLED.error_message());
  EXPECT_EQ("CANCELLED", Status::CANCELLED.ToString());
}

TEST(StatusTest, ConstantUnknown) {
  EXPECT_FALSE(Status::UNKNOWN.ok());
  EXPECT_EQ(error::UNKNOWN, Status::UNKNOWN.error_code());
  EXPECT_EQ("", Status::UNKNOWN.error_message());
  EXPECT_EQ("UNKNOWN", Status::UNKNOWN.ToString());
}

TEST(StatusTest, CustomCodeAndEmptyMessage) {
  Status s(error::NOT_FOUND, "");
  EXPECT_EQ(error::NOT_FOUND, s.error_code());
  EXPECT_EQ("", s.error_message());
  EXPECT_EQ("NOT_FOUND", s.ToString());
}

TEST(StatusTest, CustomCodeAndMessage) {
  Status s(error::NOT_FOUND, "Nothing here");
  EXPECT_EQ(error::NOT_FOUND, s.error_code());
  EXPECT_EQ("Nothing here", s.error_message());
  EXPECT_EQ("NOT_FOUND: Nothing here", s.ToString());
}

TEST(StatusTest, Equality) {
  Status s1(error::NOT_FOUND, "Nothing here");
  Status s2(error::NOT_FOUND, "Nothing here");
  EXPECT_EQ(s1, s2);
  Status s3(error::NOT_FOUND, "Nothing here.");
  EXPECT_NE(s1, s3);
  EXPECT_EQ(s1.error_code(), s3.error_code());
  Status s4(error::INTERNAL, "Nothing here");
  EXPECT_EQ(s1.error_message(), s2.error_message());
  EXPECT_NE(s1, s4);
}

TEST(StatusTest, CopyConstruction) {
  Status s1(error::NOT_FOUND, "Nothing here");
  Status s2(error::NOT_FOUND, "Nothing here");
  Status s3(s2);
  EXPECT_EQ(s2, s3);
  EXPECT_EQ(s1, s3);
}

TEST(StatusTest, Assignment) {
  Status s1(error::NOT_FOUND, "Nothing here");
  Status s2;
  {
    Status s3(error::NOT_FOUND, "Nothing here");
    s2 = s3;
  }
  EXPECT_EQ(s1, s2);
}

TEST(StatusTest, Streamification) {
  Status s(error::NOT_FOUND, "Nothing here");
  std::stringstream ss;
  ss << s;
  const std::string stream_output = ss.rdbuf()->str();
  EXPECT_EQ(s.ToString(), stream_output);
}

}  // namespace util
