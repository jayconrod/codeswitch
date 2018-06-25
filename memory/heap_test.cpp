// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "heap.h"
#include "vm/vm.h"

namespace codeswitch {
namespace internal {

TEST(Allocate) {
  VM vm;
  VMScope vmScope(&vm);
  auto heap = vm.heap();
  // word_t sizes[] = {1, 7, 31, 65, 256, 555, 2001, 62000};
  word_t sizes[] = {1};
  for (auto s : sizes) {
    try {
      auto addr = heap->allocate(s);
      ASSERT_TRUE(addr != 0);
    } catch (AllocationError& err) {
      t.errorf("error allocating size %d", static_cast<int>(s));
    }
  }
}

}  // namespace internal
}  // namespace codeswitch
