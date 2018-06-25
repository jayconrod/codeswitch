// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "heap.h"

#include <mutex>
#include <vector>
#include "block.h"

using std::lock_guard;
using std::max;
using std::move;
using std::mutex;
using std::unique_ptr;
using std::vector;

namespace codeswitch {
namespace internal {

thread_local Heap::Allocator Heap::allocator_ = {0};

void* Heap::allocateSlow(word_t size) {
  lock_guard<mutex> lock(mut_);
  freeAllocator(&allocator_);
  while (true) {  // retry
    if (fillAllocator(&allocator_, size)) {
      return reinterpret_cast<void*>(allocator_.allocate(size));
    }
    // TODO: collect garbage.
    addChunk();

    // Retry.
  }
}

void Heap::recordWrite(address from, address to) {}

void Heap::collectGarbage() {}

void Heap::enter() {}

void Heap::leave() {
  freeAllocator(&allocator_);
}

void Heap::freeAllocator(Allocator* alloc) {
  alloc->next = 0;
  alloc->size = 0;
  // TODO: release memory reserved by allocator
}

bool Heap::fillAllocator(Allocator* alloc, word_t size) {
  if (size < kDefaultAllocatorSize) {
    size = kDefaultAllocatorSize;
  }
  for (auto& chunk : chunks_) {
    auto addr = chunk->allocate(size);
    if (addr != 0) {
      allocator_.next = addr;
      allocator_.size = size;
      return true;
    }
  }
  return false;
}

void Heap::addChunk() {
  unique_ptr<Chunk> chunk(new Chunk);
  auto free = new (reinterpret_cast<void*>(chunk->data()), chunk->dataSize()) Free(nullptr);
  chunk->setFreeListHead(free);

  lock_guard<mutex> lock(mut_);
  chunks_.emplace_back(move(chunk));
}

}  // namespace internal
}  // namespace codeswitch
