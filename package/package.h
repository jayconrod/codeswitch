// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_package_h
#define package_package_h

#include "data/array.h"
#include "function.h"
#include "memory/heap.h"
#include "memory/ptr.h"

namespace codeswitch {
namespace internal {

class Package {
 public:
  explicit Package(Array<Ptr<Function>>* functions) : functions_(functions) {}
  static Package* make(Array<Ptr<Function>>* functions) {
    return new (heap.allocate(sizeof(Package))) Package(functions);
  }

 private:
  Ptr<Array<Ptr<Function>>> functions_;
};

}  // namespace internal
}  // namespace codeswitch

#endif
