// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef test_test_h
#define test_test_h

#include <sstream>
#include <string>
#include <vector>

namespace codeswitch {
namespace internal {

class Test;
class TestCase;
class TestRunner;

class TestRunner {
 public:
  bool Run();
};

class Test {
 public:
  explicit Test(const char* name) : name_(name) {}
  ~Test();

  void error(const std::string& msg);
  void errorf(const char* fmt, ...);
  void fatal(const std::string& msg);
  void fatalf(const char* msg, ...);

  bool passed() const { return passed_; }
  void reportFail();

 private:
  const char* name_;
  bool passed_ = true;
};

#define ASSERT_TRUE(exp)                                                   \
  do {                                                                     \
    if (!(exp)) {                                                          \
      std::stringstream ss;                                                \
      ss << __FILE__ << ":" << __LINE__ << ": " << #exp << " is not true"; \
      t.fatal(ss.str());                                                   \
    }                                                                      \
  } while (false)

#define ASSERT_FALSE(exp)                                                   \
  do {                                                                      \
    if (exp) {                                                              \
      std::stringstream ss;                                                 \
      ss << __FILE__ << ":" << __LINE__ << ": " << #exp << " is not false"; \
      t.fatal(ss.str());                                                    \
    }                                                                       \
  } while (false)

#define ASSERT_EQ(lexp, rexp)                                        \
  do {                                                               \
    auto l = (lexp);                                                 \
    auto r = (rexp);                                                 \
    if (l != r) {                                                    \
      std::stringstream ss;                                          \
      ss << __FILE__ << ":" << __LINE__ << ": " << l << " != " << r; \
      t.fatal(ss.str());                                             \
    }                                                                \
  } while (false)

class TestCase {
 public:
  TestCase(const char* name, void (*fn)(Test&));

  const char* name;
  void (*fn)(Test&);
};

#define TEST(name)                          \
  void name(Test& t);                       \
  TestCase _##name##TestCase(#name, &name); \
  void name(Test& t)

extern std::vector<TestCase> testCases;

}  // namespace internal
}  // namespace codeswitch

#endif
