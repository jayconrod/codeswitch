// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "roots.h"

#include "block.h"

namespace codeswitch {
namespace internal {

Roots::Roots(Heap* heap) {
  metaMeta_ = new (heap, 0, sizeof(Meta), sizeof(word_t)) Meta(kMetaBlockType);
}

}  // namespace internal
}  // namespace codeswitch
