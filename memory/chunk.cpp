// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "chunk.h"

#include <mutex>

#include "block.h"
#include "platform/platform.h"

using std::lock_guard;
using std::mutex;

namespace codeswitch {
namespace internal {

void* Chunk::operator new(size_t size) {
  ASSERT(size == sizeof(Chunk));
  return reinterpret_cast<void*>(allocateChunk(size, size));
}

void Chunk::operator delete(void* addr) {
  freeChunk(addr, sizeof(Chunk));
}

address Chunk::allocate(word_t size) {
  lock_guard<mutex> lock(mut_);
  for (Free** prev = &freeListHead_; *prev != nullptr; prev = &(*prev)->next_) {
    auto free = *prev;
    if (free->size_ < size) {
      continue;
    }
    auto addr = reinterpret_cast<address>(free);
    auto remaining = free->size_ - size;
    if (remaining >= kMinFreeSize) {
      auto newFreeAddr = addr + size;
      auto newFree = new (reinterpret_cast<void*>(newFreeAddr), remaining) Free(free->next());
      *prev = newFree;
    }
    return addr;
  }
  return 0;
}

}  // namespace internal
}  // namespace codeswitch
