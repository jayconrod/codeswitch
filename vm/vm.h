// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef vm_h
#define vm_h

#include <memory>

#include "common/common.h"

namespace codeswitch {
namespace internal {

class Heap;
class Roots;

class VM {
 public:
  NON_COPYABLE(VM)
  VM();

  Heap* heap() const { return heap_.get(); }
  Roots* roots() const { return roots_.get(); }

 private:
  std::unique_ptr<Heap> heap_;
  std::unique_ptr<Roots> roots_;
};

}  // namespace internal
}  // namespace codeswitch

#endif
