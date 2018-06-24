// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "block.h"

#include "heap.h"

namespace codeswitch {
namespace internal {

void* Meta::operator new(size_t, Heap* heap, length_t dataLength, word_t objectSize, word_t elementSize) {
  auto size = sizeof(Meta) + sizeof(word_t) * dataLength;
  return heap->allocate(size);
}

}  // namespace internal
}  // namespace codeswitch
