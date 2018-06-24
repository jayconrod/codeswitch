// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef vm_h
#define vm_h

#include <memory>

#include "common/common.h"

namespace codeswitch {
namespace internal {

class HandleStorage;
class Heap;
class Roots;

class VM {
 public:
  NON_COPYABLE(VM)
  VM();
  ~VM();

  HandleStorage* handleStorage() const { return handleStorage_.get(); }
  Heap* heap() const { return heap_.get(); }
  Roots* roots() const { return roots_.get(); }

  void enter();
  void leave();
  static VM* current();

 private:
  static thread_local VM* current_;
  std::unique_ptr<HandleStorage> handleStorage_;
  std::unique_ptr<Heap> heap_;
  std::unique_ptr<Roots> roots_;
};

class VMScope {
 public:
  explicit VMScope(VM* vm) : vm_(vm) { vm_->enter(); }
  ~VMScope() { vm_->leave(); }
  NON_COPYABLE(VMScope)
 private:
  VM* vm_;
};

}  // namespace internal
}  // namespace codeswitch

#endif
