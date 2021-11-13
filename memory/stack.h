// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_stack_h
#define memory_stack_h

#include "common/common.h"

namespace codeswitch {

class Function;
class Inst;
class Package;

struct Frame {
  Frame* fp;
  const Inst* ip;
  Function* fn;
  Package* pp;
};

class Stack {
 public:
  Stack();
  ~Stack();
  NON_COPYABLE(Stack)

  address start() const { return start_; }
  address limit() const { return limit_; }
  inline void check(length_t n);

  Frame* frame() const { return reinterpret_cast<Frame*>(fp); }

  template <class T>
  void push(const T& v);
  template <class T>
  T pop();
  template <class T>
  T& at(intptr_t i);

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

template <class T>
void Stack::push(const T& v) {
  sp -= sizeof(T);
  *reinterpret_cast<T*>(sp) = v;
}

template <class T>
T Stack::pop() {
  auto v = *reinterpret_cast<T*>(sp);
  sp += sizeof(T);
  return v;
}

template <class T>
T& Stack::at(intptr_t i) {
  auto p = sp + i * kWordSize;
  return *reinterpret_cast<T&>(p);
}

}  // namespace codeswitch

#endif
