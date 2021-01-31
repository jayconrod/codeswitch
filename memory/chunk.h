// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_chunk_h
#define memory_chunk_h

#include <cstddef>
#include <iterator>
#include <mutex>

#include "common/common.h"

#include "bitmap.h"

namespace codeswitch {
namespace internal {

class Free;
class VM;

/**
 * Blocks are aligned to 8 bytes all architectures. We need a reasonably large
 * alignment so that the marking bitmap (which has one bit per possible block
 * start address) doesn't take up too much space, but not too large an alignment
 * to waste space.
 */
const word_t kBlockAlignment = 8;

/**
 * The maximum block size is set so there isn't too much waste at the end of a
 * chunk.
 *
 * TODO: support larger allocations in mmap'd blocks with their own pointer
 * maps.
 */
const word_t kMaxBlockSize = 128 * KB;

/**
 * A Chunk is an aligned region of memory allocated from the kernel using mmap
 * or a similar mechanism.
 *
 * The heap is composed of chunks. Each chunk contains a header, an allocation
 * bitmap (one bit per block), a marking bitmap (one bit per block), a pointer
 * bitmap (one bit per word), and a set of blocks of fixed size that can be
 * allocated.
 */
class Chunk {
 public:
  NON_COPYABLE(Chunk)

  static const word_t kSize = 1 * MB;
  void* operator new(size_t size);
  void operator delete(void* addr);
  explicit Chunk(word_t blockSize);

  static Chunk* fromAddress(const void* p);
  static Chunk* fromAddress(address addr);

  word_t blockSize() const { return blockSize_; }
  address blockContaining(address p);

  address allocate();

 private:
  // Header section. Make sure kHeaderSize matches.

  /** mu_ guards the header and bitmap. */
  std::mutex mu_;

  /**
   * blockSize_ is the size in bytes of each block. Must be a multiple of
   * kBlockAlignment.
   */
  word_t blockSize_;

  /**
   * free_ points to the head of a singly linked list of blocks in this chunk
   * freed by the garbage collector.
   */
  address free_;

  /**
   * nextFree_ points to the first block in the free space at the end of
   * the chunk.
   */
  address nextFree_;

  static const word_t kHeaderSize = sizeof(mu_) + sizeof(blockSize_) + sizeof(free_) + sizeof(nextFree_);

  uint8_t pad_[kSize - kHeaderSize];

  // Bitmap and data constants.
  //
  // The marking bitmap follows the header. The marking bitmap contains at least
  // two bits per 8 bytes in the data section. The low bit indicates whether
  // the corresponding block is marked (only the first word in the block is
  // marked). The high bit indicates whether the word contains a pointer and
  // is set by write barriers.
  //
  // The data section follows the marking bitmap. It does not contain any
  // heap metadata.
  static const word_t kDataOffset = 16 * KB;
  static const word_t kBitmapOffset = align(kHeaderSize, kWordSize);
  static const word_t kBitmapCount = (kDataOffset - kBitmapOffset) * 8;
  static const word_t kDataSize = kBitmapCount / 2 * kBlockAlignment;
  static const word_t kDataEndOffset = kDataOffset + kDataSize;
};

static_assert(sizeof(Chunk) == Chunk::kSize, "Chunk does not have expected size");

inline Chunk* Chunk::fromAddress(const void* p) {
  return fromAddress(reinterpret_cast<address>(p));
}

inline Chunk* Chunk::fromAddress(address addr) {
  return reinterpret_cast<Chunk*>(addr & ~(kSize - 1));
}

inline address Chunk::blockContaining(address p) {
  auto base = reinterpret_cast<address>(this) + kDataOffset;
  auto offset = p - base;
  return base + (offset / blockSize_ * blockSize_);
}

/** Attempts to allocate a free block. Returns 0 if no blocks are free. */
inline address Chunk::allocate() {
  std::lock_guard<std::mutex> lock(mu_);
  if (free_ != 0) {
    auto block = free_;
    auto next = reinterpret_cast<address*>(free_);
    free_ = *next;
    *next = 0;
    return block;
  }

  if (nextFree_ != 0) {
    auto block = nextFree_;
    auto next = nextFree_ + blockSize_;
    if (next - reinterpret_cast<address>(this) >= kDataEndOffset) {
      next = 0;
    }
    nextFree_ = next;
    return block;
  }

  return 0;
}

}  // namespace internal
}  // namespace codeswitch

#endif
