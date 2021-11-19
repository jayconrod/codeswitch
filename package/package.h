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

class Package {
 public:
  explicit Package(List<Ptr<Function>>* functions) : functions_(*functions) {}
  static Package* make(List<Ptr<Function>>* functions) {
    return new (heap.allocate(sizeof(Package))) Package(functions);
  }

  List<Ptr<Function>>& functions() { return functions_; }
  const List<Ptr<Function>>& functions() const { return functions_; }
  Function* findFunction(const String& name);

 private:
  List<Ptr<Function>> functions_;
};

}  // namespace codeswitch

#endif
