// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "handle.h"

#include <deque>
#include <mutex>
#include "common/common.h"

namespace codeswitch {

HandleStorage handleStorage;

uintptr_t HandleStorage::allocSlot() {
  std::lock_guard<std::mutex> lock(mu_);
  if (free_ != 0) {
    auto slot = free_;
    auto next = *reinterpret_cast<uintptr_t*>(free_) & ~static_cast<uintptr_t>(1);
    free_ = next;
    *reinterpret_cast<uintptr_t*>(slot) = 0;
    return slot;
  }
  slots_.push_back(0);
  return reinterpret_cast<uintptr_t>(&slots_.back());
}

void HandleStorage::freeSlot(uintptr_t slot) {
  std::lock_guard<std::mutex> lock(mu_);
  *reinterpret_cast<uintptr_t*>(slot) = free_ | 1;
  free_ = slot;
}

}  // namespace codeswitch
