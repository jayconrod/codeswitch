// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "vm.h"

#include "memory/heap.h"
#include "memory/roots.h"

namespace codeswitch {
namespace internal {

VM::VM() : heap_(new Heap), roots_(new Roots(heap_.get())) {}

}  // namespace internal
}  // namespace codeswitch
