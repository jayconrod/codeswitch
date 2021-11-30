// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test.h"

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <iostream>
#include <vector>

#include "common/common.h"
#include "flag/flag.h"

int main(int argc, char* argv[]) {
  codeswitch::FlagSet flags("test", "test [-run=testname]");
  codeswitch::TestRunner runner;
  flags.stringFlag(&runner.filter, "run", "", "name of test to run (all tests are run by default)", false);
  try {
    flags.parse(argc - 1, argv + 1);
  } catch (std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  return runner.Run() ? 0 : 1;
}

namespace codeswitch {

std::vector<TestCase> testCases;

class TestFatal {};

bool TestRunner::Run() {
  abortThrowException = true;
  abortBacktrace = true;
  bool passedAll = true;
  for (auto tc : testCases) {
    if (!filter.empty() && filter != tc.name) {
      continue;
    }

    Test t(tc.name);
    try {
      tc.fn(t);
    } catch (TestFatal) {
    } catch (std::exception& x) {
      t.fatal(x.what());
    }
    if (!t.passed()) {
      passedAll = false;
    }
  }
  return passedAll;
}

Test::~Test() {
  if (passed_) {
    std::cerr << "PASS: " << name_ << std::endl;
  }
}

void Test::error(const std::string& msg) {
  reportFail();
  std::cerr << msg << std::endl;
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
  std::cerr << msg << std::endl;
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
  std::cerr << "FAIL: " << name_ << std::endl;
}

TestCase::TestCase(const char* name, void (*fn)(Test& t)) : name(name), fn(fn) {
  testCases.push_back(*this);
}

}  // namespace codeswitch
