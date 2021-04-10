// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "str.h"

#include <cstdarg>
#include <cstdio>
#include <string>

namespace codeswitch {
namespace internal {

std::string strprintf(const char* fmt, ...) {
  std::string s;
  va_list args;
  va_start(args, fmt);
  auto n = vsnprintf(nullptr, 0, fmt, args);
  s.resize(n + 1);
  vsnprintf(&s.front(), n, fmt, args);
  s.resize(n);
  va_end(args);
  return s;
}

}  // namespace internal
}  // namespace codeswitch
