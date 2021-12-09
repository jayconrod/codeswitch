// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_heap_h
#define memory_heap_h

#include <deque>
#include <functional>
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

/** Initial allocation threshold for triggering the garbage collector. */
const uintptr_t kInitialAllocationLimit = 1 * MB;

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
  void recordWrite(uintptr_t from, uintptr_t to);

  template <class T>
  void recordWrite(T** from, T* to);

  static void checkBound(uintptr_t base, uintptr_t offset);
  static uintptr_t blockContaining(uintptr_t p);
  static uintptr_t blockSize(uintptr_t p);

  static bool isPointer(uintptr_t addr);
  static void setPointer(uintptr_t addr);
  static bool isMarked(uintptr_t addr);
  static void setMarked(uintptr_t addr);

  /** Reclaim memory used by blocks that are no longer reachable. */
  void collectGarbage();

  /**
   * Prevent (or allow) garbage collection. This shouldn't be used often, but
   * it may be useful when performing a sequence of unsafe allocations.
   */
  void setGCLock(bool locked);

  /**
   * Registers an "accept" function that may be called with a "visit" function.
   * The "accept" function should call the "visit" function on a set of
   * addresses that point into the heap, forming the roots of the pointer graph.
   */
  void registerRoots(std::function<void(std::function<void(uintptr_t)>)> accept);

  /**
   * Completely marks the heap, then checks internal heap invariants.
   * Used for testing and debugging.
   */
  void validate();

  bool isOnHeap(uintptr_t addr);

 private:
  void collectGarbageLocked();
  void scanRootsLocked();
  void markLocked();
  void sweepLocked();

  enum class GCPhase : int {
    NONE,
    LOCKED,
  };

  std::mutex mu_;

  /**
   * Maps allocation sizes to lists of chunks holding blocks of those sizes.
   * Every block in a chunk has the same size.
   */
  std::unordered_map<size_t, std::vector<std::unique_ptr<Chunk>>> chunksBySize_;

  /**
   * Total number of bytes allocated in blocks on the heap. This only includes
   * bytes that are part of an allocated block, not free blocks, and not
   * other bookkeeping information that is part of a chunk.
   */
  uintptr_t bytesAllocated_ = 0;

  /**
   * When bytesAllocated_ exceeds this limit, collectGarbageLocked should
   * be called. That may perform part of an incremental collection, depending
   * on the heap and GC state.
   */
  uintptr_t allocationLimit_ = kInitialAllocationLimit;

  /**
   * List of "accept" functions registered with registerRoots. scanRootsLocked
   * calls these with a function that adds unmarked roots to markStack_.
   */
  std::vector<std::function<void(std::function<void(uintptr_t)>)>> rootAcceptors_;

  GCPhase gcPhase_ = GCPhase::NONE;

  /**
   * Holds addresses on the heap that contain pointers to potentially unmarked
   * blocks. recordWrite and scanRoots push pointers here. markIncremental
   * pushes and pops pointers as it traverses the block graph.
   */
  std::deque<uintptr_t> markStack_;
};

extern Heap* heap;

template <class T>
void Heap::recordWrite(T** from, T* to) {
  recordWrite(reinterpret_cast<uintptr_t>(from), reinterpret_cast<uintptr_t>(to));
}

}  // namespace codeswitch

#endif
