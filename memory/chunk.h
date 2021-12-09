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

class Free;
class VM;

/**
 * Blocks are aligned to 8 bytes all architectures. We need a reasonably large
 * alignment so that the marking bitmap (which has one bit per possible block
 * start uintptr_t) doesn't take up too much space, but not too large an alignment
 * to waste space.
 */
const uintptr_t kBlockAlignment = 8;

/**
 * The maximum block size is set so there isn't too much waste at the end of a
 * chunk.
 *
 * TODO: support larger allocations in mmap'd blocks with their own pointer
 * maps.
 */
const uintptr_t kMaxBlockSize = 128 * KB;

/**
 * A Chunk is an aligned region of memory allocated from the kernel using mmap
 * or a similar mechanism. A chunk holds blocks of the same size, which may
 * be allocated by the heap.
 *
 * The heap is composed of chunks. Each chunk contains a header, a pointer
 * bitmap (one bit per word) indicating which words contain pointers, a
 * marking bitmap (one bit per word, though only the first bit for each
 * block is used) used by the garbage collector, and a contiguous data
 * section comprising blocks of the same size.
 * 
 * Within the data section, each chunk has a free list and a free section.
 * The free list is a singly linked list of free blocks. The first word in
 * each block is a pointer to the next free block or 0 if there are no more.
 * Other words in free blocks are 0, and pointer and mark bits are clear.
 * The free section is a contiguous set of free blocks at the end of the
 * chunk. All words in the free section are zero, and pointer and mark bits
 * are clear. When a chunk is initially allocated, the free section takes up
 * the whole data section. Ideally, it doesn't even need physical pages
 * backing it.
 */
class Chunk {
 public:
  NON_COPYABLE(Chunk)

  static const uintptr_t kSize = 1 * MB;
  void* operator new(size_t size);
  void operator delete(void* addr);
  explicit Chunk(uintptr_t blockSize);

  static Chunk* fromAddress(const void* p) { return fromAddress(reinterpret_cast<uintptr_t>(p)); }

  /**
   * Returns the chunk containing a given address, assuming it belongs to
   * a chunk. Chunks are aligned in memory, so this works by masking off
   * the low bits of the address.
   */
  static Chunk* fromAddress(uintptr_t addr);

  /**
   * Returns the size of blocks allocated from this chunk. All blocks within
   * the chunk are the same size.
   */
  uintptr_t blockSize() const { return blockSize_; }

  /** Returns the address of a block containing a given address. */
  uintptr_t blockContaining(uintptr_t addr);

  /**
   * Returns the number of bytes allocated from this chunk. This doesn't
   * include chunk overhead like the header and bitmaps. It also doesn't
   * count free blocks.
   */
  uintptr_t bytesAllocated() const { return bytesAllocated_; }

  /**
   * Allocates an unused block, either from the free list or free section
   * and returns it. Returns 0 if there are no unallocated blocks.
   */
  uintptr_t allocate();

  /**
   * Returns whether an address has been marked as a pointer
   * with setPointer. addr must be a word-aligned address on this chunk.
   */
  bool isPointer(uintptr_t addr);

  /**
   * Marks an address as a pointer. addr must be a word-aligned address
   * on this chunk.
   */
  void setPointer(uintptr_t addr);

  /**
   * Returns whether an address has been marked as live with setMarked. addr
   * must be the address of a block on this chunk.
   */
  bool isMarked(uintptr_t addr);

  /**
   * Marks an address as live. addr must be the address of a block on
   * this chunk.
   */
  void setMarked(uintptr_t addr);

  /** Returns whether any block on this chunk has been marked as live. */
  bool hasMark();

  /**
   * Frees blocks on this chunk not marked as live. Any blocks not marked
   * with setMarked are added to the free list or the free section at the end.
   * Their contents are zeroed, and their pointer bits are cleared.
   * All mark bits are cleared. The number of bytes allocated is recalculated.
   */
  void sweep();

  /** Checks heap invariants on this chunk. Used for debugging and testing. */
  void validate();

  // Bitmap and data constants.
  //
  // The header is followed by two bitmaps, taking the first 32 KiB of the
  // chunk. Actually the bitmaps overlap with the header: each bit corresponds
  // to a word in the chunk, and the bits corresponding to the words of the
  // bitmaps themselves are not used.
  //
  // The first bitmap contains bits indicating which words in the chunk are
  // pointers. These are set by write barriers.
  //
  // The second bitmap contains marking bits set by the garbage collector.
  // sweep frees unmarked blocks.
  static const uintptr_t kWordsInChunk = kSize / kWordSize;
  static const uintptr_t kBitmapSizeInBytes = kWordsInChunk * 2 / 8;
  static const uintptr_t kDataOffset = kBitmapSizeInBytes;
  static const uintptr_t kDataSize = kSize - kDataOffset;

 private:
  Bitmap pointerBitmapLocked();
  Bitmap markBitmapLocked();
  bool isPointerLocked(uintptr_t addr);
  bool isMarkedLocked(uintptr_t addr);

  // Header section. Make sure kHeaderSize matches.

  /** mu_ guards the header and bitmap. */
  std::mutex mu_;

  /**
   * Size in bytes of all blocks on this chunk. Must be a multiple of
   * kBlockAlignment.
   */
  uintptr_t blockSize_;

  /**
   * Bytes allocated on this chunk. Used for the garbage collector's accounting.
   */
  uintptr_t bytesAllocated_;

  /**
   * Head of a singly linked list of blocks freed by the garbage collector.
   * Should be allocated before free space.
   */
  uintptr_t freeList_;

  /**
   * Address of a contiguous set of free blocks at the end of the chunk.
   * Initially, this takes up the entire chunk. Always contains zeroes with
   * no pointer or mark bits set.
   */
  uintptr_t freeSpace_;

  static const uintptr_t kHeaderSize =
      sizeof(mu_) + sizeof(blockSize_) + sizeof(bytesAllocated_) + sizeof(freeList_) + sizeof(freeSpace_);

  uint8_t pad_[kSize - kHeaderSize];
};

static_assert(sizeof(Chunk) == Chunk::kSize, "Chunk does not have expected size");

inline Chunk* Chunk::fromAddress(uintptr_t addr) {
  return reinterpret_cast<Chunk*>(addr & ~(kSize - 1));
}

inline uintptr_t Chunk::blockContaining(uintptr_t p) {
  auto base = reinterpret_cast<uintptr_t>(this) + kDataOffset;
  auto offset = p - base;
  return base + (offset / blockSize_ * blockSize_);
}

/** Attempts to allocate a free block. Returns 0 if no blocks are free. */
inline uintptr_t Chunk::allocate() {
  std::lock_guard<std::mutex> lock(mu_);
  if (freeList_ != 0) {
    auto block = freeList_;
    auto next = reinterpret_cast<uintptr_t*>(freeList_);
    freeList_ = *next;
    *next = 0;
    bytesAllocated_ += blockSize_;
    return block;
  }

  if (freeSpace_ + blockSize_ <= reinterpret_cast<uintptr_t>(this) + kSize) {
    auto block = freeSpace_;
    freeSpace_ += blockSize_;
    bytesAllocated_ += blockSize_;
    return block;
  }

  return 0;
}

inline Bitmap Chunk::pointerBitmapLocked() {
  auto base = reinterpret_cast<uintptr_t*>(this);
  return Bitmap(base, kBitmapSizeInBytes * 8 / 2);
}

inline Bitmap Chunk::markBitmapLocked() {
  auto base = reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + kBitmapSizeInBytes / 2);
  return Bitmap(base, kBitmapSizeInBytes * 8 / 2);
}

inline bool Chunk::isPointer(uintptr_t addr) {
  std::lock_guard lock(mu_);
  return isPointerLocked(addr);
}

inline void Chunk::setPointer(uintptr_t addr) {
  std::lock_guard lock(mu_);
  auto index = (addr - reinterpret_cast<uintptr_t>(this)) / kWordSize;
  pointerBitmapLocked().set(index, true);
}

inline bool Chunk::isMarked(uintptr_t addr) {
  std::lock_guard lock(mu_);
  return isMarkedLocked(addr);
}

inline void Chunk::setMarked(uintptr_t addr) {
  std::lock_guard lock(mu_);
  auto index = (addr - reinterpret_cast<uintptr_t>(this)) / kWordSize;
  markBitmapLocked().set(index, true);
}

inline bool Chunk::isPointerLocked(uintptr_t addr) {
  auto index = (addr - reinterpret_cast<uintptr_t>(this)) / kWordSize;
  return pointerBitmapLocked()[index];
}

inline bool Chunk::isMarkedLocked(uintptr_t addr) {
  auto index = (addr - reinterpret_cast<uintptr_t>(this)) / kWordSize;
  return markBitmapLocked()[index];
}

}  // namespace codeswitch

#endif
