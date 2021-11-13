// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "package.h"

namespace codeswitch {

Function* Package::findFunction(String* name) {
  for (auto& fn : functions_) {
    if (*fn->name() == *name) {
      return fn.get();
    }
  }
  return nullptr;
}

}  // namespace codeswitch