// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "stack.h"

namespace codeswitch {
namespace internal {

// TODO: dynamic stack size.
const length_t kStackSize = 4096;

Stack::Stack() {
  limit_ = reinterpret_cast<address>(new uint8_t[kStackSize]);
  start_ = limit_ + kStackSize;
  sp = start_;
  fp = start_;
}

Stack::~Stack() {
  delete[] reinterpret_cast<uint8_t*>(limit_);
}

}  // namespace internal
}  // namespace codeswitch
