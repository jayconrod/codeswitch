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

  // If we've reached the allocation threshold, collect garbage first.
  std::lock_guard<std::mutex> lock(mu_);
  if (bytesAllocated_ + blockSize >= allocationLimit_) {
    collectGarbageLocked();
  }
  bytesAllocated_ += blockSize;

  // Try to allocate from each chunk of the correct size.
  // OPT: track which chunks have free space.
  auto& chunks_ = chunksBySize_[blockSize];
  for (auto& c : chunks_) {
    auto block = c->allocate();
    if (block != 0) {
      return reinterpret_cast<void*>(block);
    }
  }

  // Create a new chunk, add it to the list, then allocate from that.
  chunks_.emplace_back(new Chunk(blockSize));
  auto block = chunks_.back()->allocate();
  return reinterpret_cast<void*>(block);
}

void Heap::recordWrite(uintptr_t from, uintptr_t to) {
  // OPT: don't lock the heap here. This will be extremely slow.
  std::lock_guard lock(heap->mu_);
  setPointer(from);
}

bool Heap::isPointer(uintptr_t addr) {
  return Chunk::fromAddress(addr)->isPointer(addr);
}

void Heap::setPointer(uintptr_t addr) {
  Chunk::fromAddress(addr)->setPointer(addr);
}

bool Heap::isMarked(uintptr_t addr) {
  return Chunk::fromAddress(addr)->isMarked(addr);
}

void Heap::setMarked(uintptr_t addr) {
  return Chunk::fromAddress(addr)->setMarked(addr);
}

void Heap::checkBound(uintptr_t base, uintptr_t offset) {
  auto block = blockContaining(base);
  auto size = blockSize(block);
  if (size <= offset) {
    throw BoundsCheckError();
  }
}

uintptr_t Heap::blockContaining(uintptr_t p) {
  if (p == kZeroAllocAddress) {
    return kZeroAllocAddress;
  }
  return Chunk::fromAddress(p)->blockContaining(p);
}

uintptr_t Heap::blockSize(uintptr_t p) {
  if (p == kZeroAllocAddress) {
    return 0;
  }
  return Chunk::fromAddress(p)->blockSize();
}

void Heap::collectGarbage() {
  std::lock_guard lock(mu_);
  collectGarbageLocked();
}

void Heap::setGCLock(bool locked) {
  std::lock_guard lock(mu_);
  if (locked) {
    ASSERT(gcPhase_ == GCPhase::NONE);
    gcPhase_ = GCPhase::LOCKED;
  } else {
    ASSERT(gcPhase_ == GCPhase::LOCKED);
    gcPhase_ = GCPhase::NONE;
  }
}

void Heap::registerRoots(std::function<void(std::function<void(uintptr_t)>)> accept) {
  std::lock_guard lock(mu_);
  rootAcceptors_.push_back(accept);
}

void Heap::validate() {
  std::lock_guard lock(mu_);

  // Completely mark the heap.
  scanRootsLocked();
  markLocked();

  // Iterate over all blocks in all chunks.
  uintptr_t bytesAllocated = 0;
  for (auto& sizes : chunksBySize_) {
    for (auto& chunk : sizes.second) {
      chunk->validate();
      bytesAllocated += chunk->bytesAllocated();
    }
  }
  ASSERT(bytesAllocated == bytesAllocated_);
}

bool Heap::isOnHeap(uintptr_t addr) {
  for (auto& size : chunksBySize_) {
    for (auto& chunk : size.second) {
      if (Chunk::fromAddress(addr) == chunk.get()) {
        return true;
      }
    }
  }
  return false;
}

void Heap::collectGarbageLocked() {
  switch (gcPhase_) {
    case GCPhase::LOCKED:
      return;

    case GCPhase::NONE:
      scanRootsLocked();
      markLocked();
      sweepLocked();
      allocationLimit_ = 2 * bytesAllocated_;
      break;
  }
}

void Heap::scanRootsLocked() {
  auto visit = [this](uintptr_t p) {
    if (!isMarked(p)) {
      markStack_.push_back(p);
    }
  };
  for (auto& accept : rootAcceptors_) {
    accept(visit);
  }
}

void Heap::markLocked() {
  while (!markStack_.empty()) {
    auto slot = markStack_.back();
    markStack_.pop_back();
    auto begin = blockContaining(slot);
    auto end = begin + blockSize(begin);
    setMarked(begin);
    for (auto slot = begin; slot < end; slot += kWordSize) {
      if (isPointer(slot)) {
        auto p = *reinterpret_cast<uintptr_t*>(slot);
        if (!isMarked(p)) {
          markStack_.push_back(p);
        }
      }
    }
  }
}

void Heap::sweepLocked() {
  uintptr_t bytesAllocated = 0;
  for (auto& chunks : chunksBySize_) {
    // Free chunks with no blocks allocated.
    std::remove_if(chunks.second.begin(), chunks.second.end(), [](auto& chunk) { return !chunk->hasMark(); });

    // Clean out garbage from remaining chunks.
    for (auto& chunk : chunks.second) {
      chunk->sweep();
      bytesAllocated += chunk->bytesAllocated();
    }
  }
  bytesAllocated_ = bytesAllocated;
}

}  // namespace codeswitch
