// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "stack.h"

#include <functional>
#include "heap.h"

namespace codeswitch {

// TODO: dynamic stack size.
const size_t kStackSize = 4096;

Stack::Stack() {
  limit_ = reinterpret_cast<uintptr_t>(new uint8_t[kStackSize]);
  start_ = limit_ + kStackSize;
  sp = start_;
  fp = start_;
}

Stack::~Stack() {
  delete[] reinterpret_cast<uint8_t*>(limit_);
}

void Stack::accept(std::function<void(uintptr_t)> visit) {
  // TODO: visit other pointers on the stack. Currently, the type system doesn't
  // allow pointers, so there's actually nothing to visit. When pointers are
  // allowed, each function needs a bitmap indicating which argument words
  // contain pointers, and which stack slots contain pointers for each safe
  // point. Safe points are instructions that can trigger GC, especially
  // anything that allocates or calls.
  for (auto fr = frame(); fr != nullptr; fr = fr->fp) {
    visit(reinterpret_cast<uintptr_t>(fr->fn));
    visit(reinterpret_cast<uintptr_t>(fr->pp));
  }
}

Stack* StackPool::get() {
  ASSERT(!used_);
  used_ = true;
  return &stack_;
}

StackPool* stackPool;

StackPool::StackPool() {
  heap->registerRoots(std::bind(&StackPool::accept, this, std::placeholders::_1));
}

void StackPool::put(Stack* stack) {
  ASSERT(used_ && stack == &stack_);
  used_ = false;
}

void StackPool::accept(std::function<void(uintptr_t)> visit) {
  if (!used_) {
    return;
  }
  stack_.accept(visit);
}

}  // namespace codeswitch
