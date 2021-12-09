// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "roots.h"

#include "memory/handle.h"
#include "memory/heap.h"
#include "memory/stack.h"
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
  handleStorage = new HandleStorage;
  stackPool = new StackPool;
  roots = new Roots;
}

Roots::Roots() {
  heap->setGCLock(true);
  unitType = Type::make(Type::UNIT);
  boolType = Type::make(Type::BOOL);
  int64Type = Type::make(Type::INT64);
  heap->registerRoots(std::bind(&Roots::accept, this, std::placeholders::_1));
  heap->setGCLock(false);
}

void Roots::accept(std::function<void(uintptr_t)> visit) {
  visit(reinterpret_cast<uintptr_t>(unitType));
  visit(reinterpret_cast<uintptr_t>(boolType));
  visit(reinterpret_cast<uintptr_t>(int64Type));
}

}  // namespace codeswitch
