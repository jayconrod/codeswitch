// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_roots_h
#define package_roots_h

#include "common/common.h"

namespace codeswitch {

class Type;

class Roots {
 public:
  Roots();
  NON_COPYABLE(Roots)

  Type* unitType = nullptr;
  Type* boolType = nullptr;
  Type* int64Type = nullptr;
};

extern Roots* roots;

}  // namespace codeswitch

#endif
