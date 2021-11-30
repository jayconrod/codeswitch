// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "roots.h"

#include "memory/heap.h"
#include "type.h"

namespace codeswitch {

Roots::Roots() {
  heap.gcLock();
  unitType = Type::make(Type::UNIT);
  boolType = Type::make(Type::BOOL);
  int64Type = Type::make(Type::INT64);
  heap.gcUnlock();
}

}  // namespace codeswitch
