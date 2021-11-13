// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "error.h"

#include <cstdarg>
#include "str.h"

namespace codeswitch {

Error errorf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  auto s = vstrprintf(fmt, args);
  va_end(args);
  return Error(s);
}

}  // namespace codeswitch
