// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "type.h"

#include <functional>
#include "common/common.h"

namespace codeswitch {

word_t Type::size() const {
  switch (kind_) {
    case UNIT:
      return 0;
    case BOOL:
      return 1;
    case INT64:
      return 8;
  }
  ABORT("unreachable");
  return ~0;
}

word_t Type::hash() const {
  return std::hash<int>{}(kind_);
}

}  // namespace codeswitch
