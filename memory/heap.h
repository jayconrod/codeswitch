// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef heap_h
#define heap_h

#include <memory>
#include <mutex>
#include <vector>
#include "chunk.h"
#include "common/common.h"

namespace codeswitch {
namespace internal {

/**
 * Blocks are aligned to 8 bytes all architectures. We need a reasonably large alignment so
 * that the marking bitmap (which has one bit per possible block start address) doesn't take
 * up too much space, but not too large an alignment to waste space.
 */
const word_t kBlockAlignment = 8;

/**
 * We will never allocate blocks below this address. Lesser values can signal failures or
 * encoded values.
 */
const word_t kMinAddress = 1 << 20;

/**
 * Thrown when memory can't be allocated from the heap. Has a flag that indicates whether
 * allocation should be re-attempted after garbage collection.
 */
class AllocationError : public std::exception {
 public:
  explicit AllocationError(bool retry) : shouldRetryAfterGC(retry) {}

  virtual const char* what() const noexcept override { return "allocation error"; }

  bool shouldRetryAfterGC;
};

class Heap {
 public:
  NON_COPYABLE(Heap)

  Heap() {}

  /**
   * Allocates a zero-initialized block of memory of the given size.
   *
   * @returns address of the allocated memory.
   * @throws AllocationError if the block couldn't be allocated.
   */
  inline address allocate(word_t size);

  /**
   * Notifies the garbage collector that a pointer was written into a block.
   *
   * This must be called for all pointer writes unless the block being written is freshly
   * allocated, i.e., nothing else has been allocated later and no pointer to that block has
   * been stored.
   */
  static void recordWrite(address from, address to);

  template <class T>
  static void recordWrite(T** from, T* to) {
    recordWrite(reinterpret_cast<address>(from), reinterpret_cast<address>(to));
  }

  /** Reclaim memory used by blocks that are no longer reachable. */
  void collectGarbage();

 private:
  static const word_t kDefaultAllocatorSize = 64 * KB;

  struct Allocator {
    inline bool canAllocate(word_t s);
    inline address allocate(word_t s);

    address next;
    word_t size;
  };

  address allocateSlow(word_t size);
  void freeAllocator(Allocator* alloc);
  bool fillAllocator(Allocator* alloc, word_t size);

  void addChunk();

  std::mutex mut_;
  std::vector<std::unique_ptr<Chunk>> chunks_;
  static thread_local Allocator allocator_;
};

address Heap::allocate(size_t size) {
  ASSERT(size > 0);
  size = align(size, kWordSize);
  if (size > Chunk::kMaxBlockSize) {
    // TODO: support large allocations
    throw AllocationError(false);
  }

  if (allocator_.canAllocate(size)) {
    return allocator_.allocate(size);
  }
  return allocateSlow(size);
}

bool Heap::Allocator::canAllocate(word_t s) {
  return s <= size;
}

address Heap::Allocator::allocate(word_t s) {
  auto a = next;
  next += s;
  size -= s;
  return a;
}

}  // namespace internal
}  // namespace codeswitch

#endif
