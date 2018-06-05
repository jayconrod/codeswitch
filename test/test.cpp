// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <vector>

using std::cerr;
using std::endl;
using std::vector;

int main() {
  codeswitch::internal::TestRunner runner;
  return runner.Run() ? 0 : 1;
}

namespace codeswitch {
namespace internal {

vector<TestCase> testCases;

class TestFatal {};

bool TestRunner::Run() {
  bool passedAll = true;
  for (auto tc : testCases) {
    Test t(tc.name);
    try {
      tc.fn(t);
    } catch (TestFatal) {
    }
    if (!t.passed()) {
      passedAll = false;
    }
  }
  return passedAll;
}

Test::~Test() {
  if (passed_) {
    cerr << "PASS: " << name_ << endl;
  }
}

void Test::error(const std::string& msg) {
  reportFail();
  cerr << msg << endl;
}

void Test::errorf(const char* fmt, ...) {
  char buffer[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  error(buffer);
}

void Test::fatal(const std::string& msg) {
  reportFail();
  cerr << msg << endl;
  throw TestFatal{};
}

void Test::fatalf(const char* fmt, ...) {
  char buffer[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  fatal(buffer);
}

void Test::reportFail() {
  if (!passed_) {
    return;
  }
  passed_ = false;
  cerr << "FAIL: " << name_ << endl;
}

TestCase::TestCase(const char* name, void (*fn)(Test& t)) : name(name), fn(fn) {
  testCases.push_back(*this);
}

}  // namespace internal
}  // namespace codeswitch
