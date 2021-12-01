// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "heap.h"

#include <mutex>
#include "common/common.h"

namespace codeswitch {

/**
 * The global heap is shared for the entire process. This simplifies the
 * implementation, since there's no need to carry VM or heap pointers around.
 * Due to spectre attacks, it's probably never safe to use multiple heaps
 * in different security contexts in the same process, so we just design around
 * having one heap.
 */
Heap* heap;

void* Heap::allocate(size_t size) {
  // Align the requested size.
  // OPT: limit the number of chunk sizes by increasing the alignment with
  // block size.
  if (size == 0) {
    return reinterpret_cast<void*>(kZeroAllocAddress);
  }
  auto blockSize = align(size, kBlockAlignment);
  if (blockSize > kMaxBlockSize) {
    // TODO: support large allocations.
    throw AllocationError(false);
  }

  // Try to allocate from each chunk of the correct size.
  // OPT: track which chunks have free space.
  std::lock_guard<std::mutex> lock(mu_);
  auto& chunks_ = chunksBySize_[blockSize];
  for (auto& c : chunks_) {
    auto block = c->allocate();
    if (block != 0) {
      return reinterpret_cast<void*>(block);
    }
  }

  // TODO: collect garbage when we reach some allocation threshold.

  // Create a new chunk, add it to the list, then allocate from that.
  chunks_.emplace_back(new Chunk(blockSize));
  auto block = chunks_.back()->allocate();
  return reinterpret_cast<void*>(block);
}

void Heap::recordWrite(address from, address to) {
  // TODO: implement.
}

void Heap::checkBound(address base, word_t offset) {
  auto block = blockContaining(base);
  auto size = blockSize(block);
  if (size <= offset) {
    throw BoundsCheckError();
  }
}

address Heap::blockContaining(address p) {
  if (p == kZeroAllocAddress) {
    return kZeroAllocAddress;
  }
  return Chunk::fromAddress(p)->blockContaining(p);
}

word_t Heap::blockSize(address p) {
  if (p == kZeroAllocAddress) {
    return 0;
  }
  return Chunk::fromAddress(p)->blockSize();
}

void Heap::collectGarbage() {
  // TODO: implement.
}

void Heap::gcLock() {
  // TODO: implement.
}

void Heap::gcUnlock() {
  // TODO: implement.
}

}  // namespace codeswitch
