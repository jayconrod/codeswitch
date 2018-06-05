// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "platform.h"

#include <sys/mman.h>

namespace codeswitch {
namespace internal {

void* allocateChunk(size_t size, size_t alignment) {
  // TODO: randomize the allocation address.
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void* basePtr = mmap(nullptr, size + alignment, prot, flags, -1, 0);
  if (basePtr == MAP_FAILED) {
    throw SystemAllocationError{errno};
  }
  auto base = reinterpret_cast<address>(basePtr);
  auto end = base + size + alignment;
  auto chunk = align(base, alignment);
  auto chunkEnd = chunk + size;
  if (chunk > base) {
    munmap(basePtr, chunk - base);
  }
  if (chunkEnd < end) {
    munmap(reinterpret_cast<void*>(chunkEnd), end - chunkEnd);
  }
  return reinterpret_cast<void*>(chunk);
}

void freeChunk(void* addr, size_t size) {
  munmap(addr, size);
}
}
}
