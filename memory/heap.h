// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_heap_h
#define memory_heap_h

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "chunk.h"
#include "common/common.h"

namespace codeswitch {

/**
 * We will never allocate blocks below this uintptr_t. Lesser values can signal
 * failures or encoded values.
 */
const uintptr_t kMinAddress = 1 << 20;

/** Address returned when a 0-byte allocation is requested. */
const uintptr_t kZeroAllocAddress = kMinAddress;

/**
 * Thrown when memory can't be allocated from the heap. Has a flag that
 * indicates whether allocation should be re-attempted after garbage collection.
 */
class AllocationError : public std::exception {
 public:
  explicit AllocationError(bool retry) : shouldRetryAfterGC(retry) {}

  virtual const char* what() const noexcept override { return "allocation error"; }

  bool shouldRetryAfterGC;
};

/**
 * Thrown when an uintptr_t is accessed outside of the block it was expected to
 * be in. For example, this can happen when a value is read or written off the
 * end of an array.
 */
class BoundsCheckError : public std::exception {
  virtual const char* what() const noexcept override { return "bounds check error"; }
};

class Heap {
 public:
  NON_COPYABLE(Heap)

  Heap() {}

  /**
   * Allocates a zero-initialized block of memory of the given size.
   *
   * @returns uintptr_t of the allocated memory.
   * @throws AllocationError if the block couldn't be allocated.
   */
  void* allocate(uintptr_t size);

  /**
   * Notifies the garbage collector that a pointer was written into a block.
   *
   * This must be called for all pointer writes unless the block being written
   * is freshly allocated, i.e., nothing else has been allocated later and no
   * pointer to that block has been stored.
   */
  static void recordWrite(uintptr_t from, uintptr_t to);

  template <class T>
  static void recordWrite(T** from, T* to);

  static void checkBound(uintptr_t base, uintptr_t offset);
  static uintptr_t blockContaining(uintptr_t p);
  static uintptr_t blockSize(uintptr_t p);

  /** Reclaim memory used by blocks that are no longer reachable. */
  void collectGarbage();
  void gcLock();
  void gcUnlock();

 private:
  void addChunk();

  std::mutex mu_;
  std::unordered_map<size_t, std::vector<std::unique_ptr<Chunk>>> chunksBySize_;
};

extern Heap* heap;

template <class T>
void Heap::recordWrite(T** from, T* to) {
  recordWrite(reinterpret_cast<uintptr_t>(from), reinterpret_cast<uintptr_t>(to));
}

}  // namespace codeswitch

#endif
