// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "chunk.h"

#include "platform/platform.h"

namespace codeswitch {
namespace internal {

void* Chunk::operator new(size_t size) {
  ASSERT(size == sizeof(Chunk));
  return reinterpret_cast<void*>(allocateChunk(size, size));
}

void Chunk::operator delete(void* addr) {
  freeChunk(addr, sizeof(Chunk));
}
}
}
