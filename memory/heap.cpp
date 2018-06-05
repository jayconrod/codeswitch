// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "heap.h"

#include "block.h"

using namespace std;

namespace codeswitch {
namespace internal {

address Heap::allocate(word_t size) {
  size = align(size, kWordSize);
  if (size > Chunk::kMaxBlockSize) {
    throw AllocationError(false);
  }

  // TODO: optimize this a lot.
  lock_guard lock(mut_);

  while (true) {  // retry
    // Iterate over the free list in each chunk to find a large enough contiguous range.
    for (auto& chunk : chunks_) {
      auto prevp = &chunk->freeListHead_;
      auto free = *prevp;
      while (free != nullptr) {
        if (free->size() >= size) {
          auto addr = reinterpret_cast<address>(free);
          if (free->size() - size < sizeof(Free)) {
            *prevp = free->next_;
          } else {
            auto newFreeAddr = reinterpret_cast<void*>(addr + size);
            auto newFreeSize = free->size() - size;
            free = new (newFreeAddr, newFreeSize) Free(free->next_);
            *prevp = free;
          }
          return addr;
        }
        prevp = &free->next_;
        free = free->next_;
      }
    }

    // Allocate a new chunk.
    // TODO: collect garbage.
    chunks_.emplace_back(unique_ptr<Chunk>(new Chunk));

    // Retry.
  }
}

void Heap::recordWrite(address from, address to) {}

void Heap::collectGarbage() {}
}
}
