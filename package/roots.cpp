// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "roots.h"

#include "memory/heap.h"
#include "type.h"

namespace codeswitch {

Roots* roots;

// TODO: find a better way to handle initialization.
// Ideally, heap and roots would just be global variables, but static
// initialization order is undefined behavior across translation units, so
// nothing guarantees heap would be constructed first.
// Instead, this function explicitly initializes both. It doesn't really belong
// in this translation unit, but if it was in a separate file, the linker might
// leave it out if none of its symbols are used.
__attribute__((constructor)) void init() {
  heap = new Heap;
  roots = new Roots;
}

Roots::Roots() {
  heap->gcLock();
  unitType = Type::make(Type::UNIT);
  boolType = Type::make(Type::BOOL);
  int64Type = Type::make(Type::INT64);
  heap->gcUnlock();
}

}  // namespace codeswitch
