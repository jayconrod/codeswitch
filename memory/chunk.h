// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef chunk_h
#define chunk_h

#include <cstddef>
#include <iterator>
#include <mutex>

#include "common/common.h"

#include "bitmap.h"

namespace codeswitch {
namespace internal {

class Free;

/**
 * A Chunk is an aligned region of memory allocated from the kernel using mmap or some
 * equivalent mechanism.
 *
 * The heap is composed of chunks. Each chunk contains some header information, a marking
 * bitmap (for the garbage collector), and an area where blocks can be allocated.
 */
class Chunk {
 public:
  NON_COPYABLE(Chunk)

  static const word_t kSize = 1 * MB;
  static const word_t kMaxBlockSize = kSize - 16 * KB;
  static const word_t kMinFreeSize = 64;
  void* operator new(size_t size);
  void operator delete(void* addr);
  Chunk() {}

  static Chunk* fromAddress(const void* p);
  static Chunk* fromAddress(address addr);

  address allocate(word_t size);
  Free* freeListHead() { return freeListHead_; }
  void setFreeListHead(Free* freeListHead) { freeListHead_ = freeListHead; }

  Bitmap allocBitmap() { return Bitmap(allocBitmap_, kBitmapWords * kBitsInWord); }
  Bitmap markBitmap() { return Bitmap(markBitmap_, kBitmapWords * kBitsInWord); }
  address data() { return reinterpret_cast<address>(&data_[0]); }
  word_t dataSize() { return sizeof(data_); }

 private:
  // Header fields. Make sure kHeaderSize matches.
  std::mutex mut_;
  Free* freeListHead_ = nullptr;

  static const word_t kHeaderSize = sizeof(mut_) + sizeof(freeListHead_);
  static const word_t kBitmapWords = kSize / 64 / kBitsInWord;
  static const word_t kDataWords = (kSize - kHeaderSize) / kWordSize - 2 * kBitmapWords;
  static_assert(kDataWords * kWordSize >= kMaxBlockSize, "chunk data size < kMaxBlockSize");

  // Marking bitmap
  word_t allocBitmap_[kBitmapWords];
  word_t markBitmap_[kBitmapWords];
  word_t data_[kDataWords];

  friend class Heap;
};

static_assert(sizeof(Chunk) == Chunk::kSize, "Chunk does not have expected size");

inline Chunk* Chunk::fromAddress(const void* p) {
  return fromAddress(reinterpret_cast<address>(p));
}

inline Chunk* Chunk::fromAddress(address addr) {
  return reinterpret_cast<Chunk*>(addr & (kSize - 1));
}
}  // namespace internal
}  // namespace codeswitch

#endif
