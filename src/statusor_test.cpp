#include <common/util/statusor.h>

#include <cstring>
#include <memory>
#include <string>

#include <common/util/status.h>
#include <gtest/gtest.h>

using ::std::string;
using ::std::unique_ptr;

namespace util {

class MoveOnlyInt {
 public:
  MoveOnlyInt(int i) : i_(i) {}
  MoveOnlyInt(const MoveOnlyInt& other) = delete;
  MoveOnlyInt(MoveOnlyInt&& other) : i_(other.i_) {
    other.i_ = -1;
  }
  const MoveOnlyInt& operator=(const MoveOnlyInt& other) = delete;
  MoveOnlyInt& operator=(MoveOnlyInt&& other) {
    i_ = other.i_;
    other.i_ = -1;
    return *this;
  }

  int i() const { return i_; }

 private:
  int i_;
};

TEST(StatsOrTest, DefaulConstructedUnknown) {
  const StatusOr<bool> s;
  EXPECT_FALSE(s.ok());
  EXPECT_EQ(Status::UNKNOWN, s.status());
}

TEST(StatsOrTest, ConstructionFromStatus) {
  const StatusOr<bool> s(Status(error::NOT_FOUND, "Missing"));
  EXPECT_FALSE(s.ok());
  EXPECT_EQ(Status(error::NOT_FOUND, "Missing"), s.status());
  EXPECT_DEATH({s.ValueOrDie();}, "ok()");
}

TEST(StatsOrTest, ConstructionFromStatusOkDisallowedCausesDeath) {
  EXPECT_DEATH({ StatusOr<bool> s(Status::OK); }, "status.ok");
}

TEST(StatsOrTest, CopyConstructionFromValue) {
  const StatusOr<string> s("OHAI");
  ASSERT_TRUE(s.ok());
  EXPECT_EQ(Status::OK, s.status());
  EXPECT_EQ("OHAI", s.ValueOrDie());
}

TEST(StatsOrTest, MoveConstructionFromValue) {
  unique_ptr<string> sp(new string("OHAI"));
  const string* spv = sp.get();
  StatusOr<unique_ptr<string>> s(std::move(sp));
  EXPECT_TRUE(sp.get() == nullptr);
  ASSERT_TRUE(s.ok());
  EXPECT_EQ(Status::OK, s.status());
  EXPECT_TRUE(s.ValueOrDie().get() == spv);
  unique_ptr<string> sp2 = s.MoveValueOrDie();
  ASSERT_TRUE(sp2.get() != nullptr);
  EXPECT_EQ("OHAI", *sp2);
  EXPECT_TRUE(sp2.get() == spv);
  EXPECT_EQ(Status::UNKNOWN, s.status());
}

TEST(StatsOrTest, CopyConstruction) {
  const StatusOr<string> s1("OHAI");
  const StatusOr<string> s2(s1);
  EXPECT_EQ(s1.status(), s2.status());
  EXPECT_EQ("OHAI", s1.ValueOrDie());
  EXPECT_EQ("OHAI", s2.ValueOrDie());
}

TEST(StatsOrTest, MoveConstruction) {
  unique_ptr<string> sp(new string("OHAI"));
  const string* spv = sp.get();
  StatusOr<unique_ptr<string>> s1(std::move(sp));
  EXPECT_EQ(Status::OK, s1.status());
  EXPECT_EQ("OHAI", *s1.ValueOrDie());
  const StatusOr<unique_ptr<string>> s2(std::move(s1));
  EXPECT_EQ(Status::UNKNOWN, s1.status());
  EXPECT_EQ(Status::OK, s2.status());
  EXPECT_TRUE(s2.ValueOrDie().get() == spv);
}

TEST(StatsOrTest, ConversionCopyConstruction) {
  const StatusOr<const char*> s1("OHAI");
  const StatusOr<string> s2(s1);
  EXPECT_EQ(s1.status(), s2.status());
  EXPECT_STREQ("OHAI", s1.ValueOrDie());
  EXPECT_EQ("OHAI", s2.ValueOrDie());
}

TEST(StatsOrTest, ConversionMoveConstruction) {
  StatusOr<int> s1(123);
  ASSERT_TRUE(s1.ok());
  ASSERT_EQ(123, s1.ValueOrDie());
  StatusOr<MoveOnlyInt> s2(std::move(s1));
  ASSERT_TRUE(s2.ok());
  const MoveOnlyInt& s2v = s2.ValueOrDie();
  EXPECT_EQ(123, s2v.i());
  EXPECT_EQ(Status::UNKNOWN, s1.status());
  StatusOr<MoveOnlyInt> s3(std::move(s2));
  EXPECT_EQ(Status::UNKNOWN, s2.status());
  ASSERT_TRUE(s3.ok());
  EXPECT_EQ(123, s3.ValueOrDie().i());
  EXPECT_EQ(-1, s2v.i());
}

TEST(StatsOrTest, CopyAssignment) {
  const StatusOr<string> s1("OHAI");
  StatusOr<string> s2;
  s2 = s1;
  EXPECT_EQ(s1.status(), s2.status());
  EXPECT_EQ("OHAI", s1.ValueOrDie());
  EXPECT_EQ("OHAI", s2.ValueOrDie());
}

TEST(StatsOrTest, MoveAssignment) {
  unique_ptr<string> sp(new string("OHAI"));
  const string* spv = sp.get();
  StatusOr<unique_ptr<string>> s1(std::move(sp));
  EXPECT_EQ(Status::OK, s1.status());
  StatusOr<unique_ptr<string>> s2;
  s2 = std::move(s1);
  EXPECT_EQ(Status::OK, s2.status());
  EXPECT_TRUE(s2.ValueOrDie().get() == spv);
  EXPECT_EQ(Status::UNKNOWN, s1.status());
}

TEST(StatsOrTest, ConversionCopyAssignment) {
  const StatusOr<const char*> s1("OHAI");
  StatusOr<string> s2;
  s2 = s1;
  EXPECT_EQ(s1.status(), s2.status());
  EXPECT_STREQ("OHAI", s1.ValueOrDie());
  EXPECT_EQ("OHAI", s2.ValueOrDie());
}

TEST(StatsOrTest, ConversionMoveAssignment) {
  StatusOr<int> s1(123);
  StatusOr<MoveOnlyInt> s2(456);
  EXPECT_EQ(456, s2.ValueOrDie().i());
  s2 = std::move(s1);
  EXPECT_EQ(Status::UNKNOWN, s1.status());
  EXPECT_EQ(Status::OK, s2.status());
  EXPECT_EQ(123, s2.ValueOrDie().i());
}

}  // namespace util
