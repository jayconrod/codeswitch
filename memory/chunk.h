// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef chunk_h
#define chunk_h

#include <cstddef>
#include <iterator>

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

  void* operator new(size_t size);
  void operator delete(void* addr);
  Chunk() {}

  static Chunk* fromAddress(const void* p);
  static Chunk* fromAddress(address addr);

  Bitmap markBitmap();

 private:
  // Header fields. Make sure kHeaderSize matches.
  Free* freeListHead_ = nullptr;

  static const word_t kHeaderSize = sizeof(freeListHead_);
  static const word_t kMarkBitmapWords = kSize / 64 / kBitsInWord;
  static const word_t kDataWords = (kSize - kHeaderSize) / kWordSize - kMarkBitmapWords;
  static_assert(kDataWords * kWordSize >= kMaxBlockSize, "chunk data size < kMaxBlockSize");

  // Marking bitmap
  word_t markBitmap_[kMarkBitmapWords];
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
}
}

#endif
