// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_stack_h
#define memory_stack_h

#include "common/common.h"

namespace codeswitch {
namespace internal {

class Stack {
 public:
  Stack();
  ~Stack();
  NON_COPYABLE(Stack)

  address start() const { return start_; }
  address limit() const { return limit_; }
  inline void check(length_t n);

  address sp, fp;

 private:
  address start_, limit_;
};

class StackOverflowError : public std::exception {
 public:
  virtual const char* what() const noexcept override { return "stack overflow"; }
};

void Stack::check(length_t n) {
  auto top = sp - n;
  if (top < limit_ || top >= sp) {
    throw StackOverflowError();
  }
}

}  // namespace internal
}  // namespace codeswitch

#endif
