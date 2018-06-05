// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef platform_h
#define platform_h

#include <cstring>
#include <stdexcept>
#include "common/common.h"

namespace codeswitch {
namespace internal {

class SystemAllocationError : public std::exception {
 public:
  explicit SystemAllocationError(int err) : err_(err) {}
  virtual const char* what() const noexcept override { return strerror(err_); }

 private:
  int err_;
};

/** Allocates a region of memory from the kernel with the given size and * alignment. */
void* allocateChunk(size_t size, size_t alignment);

/** Frees a region allocated with {@code allocateChunk}. */
void freeChunk(void* addr, size_t size);
}
}

#endif
