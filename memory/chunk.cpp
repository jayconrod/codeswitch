// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "chunk.h"

#include "heap.h"
#include "platform/platform.h"

namespace codeswitch {

void* Chunk::operator new(size_t size) {
  ASSERT(size == sizeof(Chunk));
  return reinterpret_cast<void*>(allocateChunk(size, size));
}

void Chunk::operator delete(void* addr) {
  freeChunk(addr, sizeof(Chunk));
}

Chunk::Chunk(uintptr_t blockSize) :
    blockSize_(blockSize), freeList_(0), freeSpace_(reinterpret_cast<uintptr_t>(this) + kDataOffset) {
  ASSERT(isAligned(blockSize, kBlockAlignment));
}

bool Chunk::hasMark() {
  std::lock_guard lock(mu_);
  auto m = markBitmapLocked();
  for (uintptr_t i = 0, n = m.wordCount(); i < n; i++) {
    if (m.wordAt(i) != 0) {
      return true;
    }
  }
  return false;
}

void Chunk::sweep() {
  std::lock_guard lock(mu_);
  auto mark = markBitmapLocked();
  auto ptr = pointerBitmapLocked();
  auto words = reinterpret_cast<uintptr_t*>(this);

  // Try to expand the free space at the end of the chunk.
  auto beginIndex = kDataOffset / kWordSize;
  auto origFreeIndex = (freeSpace_ - reinterpret_cast<uintptr_t>(this)) / kWordSize;
  auto freeIndex = origFreeIndex;
  auto wordsPerBlock = blockSize_ / kWordSize;
  while (freeIndex > beginIndex) {
    auto prevIndex = freeIndex - wordsPerBlock;
    if (mark[prevIndex]) {
      break;
    }
    freeIndex = prevIndex;
  }
  std::fill(words + freeIndex, words + origFreeIndex, 0);
  for (uintptr_t i = freeIndex; i < origFreeIndex; i++) {
    ptr.set(i, false);
  }
  freeSpace_ = reinterpret_cast<uintptr_t>(words + freeIndex);

  // Rebuild the free list.
  bytesAllocated_ = 0;
  freeList_ = 0;
  for (auto blockIndex = freeIndex - wordsPerBlock; blockIndex >= beginIndex; blockIndex -= wordsPerBlock) {
    if (mark[blockIndex]) {
      bytesAllocated_ += blockSize_;
      continue;
    }
    ptr.set(blockIndex, false);
    words[blockIndex] = freeList_;
    freeList_ = reinterpret_cast<uintptr_t>(&words[blockIndex]);
    for (uintptr_t i = 1; i < wordsPerBlock; i++) {
      ptr.set(blockIndex + i, false);
      words[blockIndex + i] = 0;
    }
  }

  // Clear the mark bits.
  // Pointer bits in freed blocks have already been cleared. Bits in live blocks
  // stay set.
  mark.clear();
}

void Chunk::validate() {
  std::lock_guard lock(mu_);

  // Validate blocks before free space.
  auto words = reinterpret_cast<uintptr_t*>(this);
  auto wordsPerBlock = blockSize_ / kWordSize;
  auto freeSpaceIndex = (freeSpace_ - reinterpret_cast<uintptr_t>(this)) / kWordSize;
  auto free = freeList_;
  uintptr_t bytesAllocated = 0;
  for (auto index = kDataOffset / kWordSize; index < freeSpaceIndex; index += wordsPerBlock) {
    auto block = reinterpret_cast<uintptr_t>(&words[index]);
    if (isMarkedLocked(block)) {
      // Allocated block.
      // Each word with pointer bit set must either be 0 or an address
      // inside another marked block on the heap.
      bytesAllocated += blockSize_;
      for (uintptr_t i = 0; i < wordsPerBlock; i++) {
        auto slot = reinterpret_cast<uintptr_t>(&words[index + i]);
        if (isPointerLocked(slot) && words[index + i] != 0) {
          auto p = words[index + i];
          ASSERT(heap->isOnHeap(p));
          auto c = Chunk::fromAddress(p);
          auto caddr = reinterpret_cast<uintptr_t>(c);
          ASSERT(caddr + Chunk::kDataOffset <= p && p <= c->freeSpace_);
          ASSERT(c->isMarked(c->blockContaining(p)));
        }
      }
    } else if (free == block) {
      // Free block.
      // Must be on free list.
      // First word should be next element of free list.
      // Other words should be 0.
      // Pointer and other mark bits should be 0.
      free = words[index];
      ASSERT(!isPointerLocked(block));
      for (uintptr_t i = 1; i < wordsPerBlock; i++) {
        ASSERT(words[index + i] == 0);
        auto addr = reinterpret_cast<uintptr_t>(&words[index + i]);
        ASSERT(!isPointerLocked(addr));
        ASSERT(!isMarkedLocked(addr));
      }
    } else {
      // Dead block.
      // Other mark bits should be 0.
      // Nothing else checked.
      for (uintptr_t i = 1; i < wordsPerBlock; i++) {
        auto addr = reinterpret_cast<uintptr_t>(&words[index + i]);
        ASSERT(!isMarkedLocked(addr));
      }
      bytesAllocated += blockSize_;
    }
  }
  ASSERT(bytesAllocated == bytesAllocated_);

  // Validate free space. Should be zeroes, no mark bits, no pointer bits.
  for (auto index = freeSpaceIndex; index < kSize / kWordSize; index++) {
    ASSERT(words[index] == 0);
    auto addr = reinterpret_cast<uintptr_t>(&words[index]);
    ASSERT(!isPointerLocked(addr));
    ASSERT(!isMarkedLocked(addr));
  }
}

}  // namespace codeswitch
