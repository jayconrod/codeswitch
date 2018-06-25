// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "vm.h"

#include "memory/handle.h"
#include "memory/heap.h"
#include "memory/roots.h"

namespace codeswitch {
namespace internal {

VM::VM() {
  current_ = this;  // enable allocation in constructors
  handleStorage_.reset(new HandleStorage());
  heap_.reset(new Heap());
  roots_.reset(new Roots(heap()));
  current_ = nullptr;
}

VM::~VM() {
  ASSERT(current_ == nullptr);
}

void VM::enter() {
  ASSERT(current_ == nullptr);
  current_ = this;
  heap_->enter();
  handleStorage_->enter();
}

void VM::leave() {
  ASSERT(current_ != nullptr);
  handleStorage_->leave();
  heap_->leave();
  current_ = nullptr;
}

VM* VM::current() {
  ASSERT(current_ != nullptr);
  return current_;
}

thread_local VM* VM::current_ = nullptr;
}  // namespace internal
}  // namespace codeswitch
