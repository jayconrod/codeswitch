// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "str.h"

#include <cstdarg>
#include <cstdio>
#include <string>

namespace codeswitch {

std::string strprintf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  auto s = vstrprintf(fmt, args);
  va_end(args);
  return s;
}

std::string vstrprintf(const char* fmt, va_list args) {
  va_list args2;
  va_copy(args2, args);
  auto n = vsnprintf(nullptr, 0, fmt, args2);
  va_end(args2);

  std::string s;
  s.resize(n + 1);
  vsnprintf(&s.front(), n + 1, fmt, args);
  s.resize(n);
  return s;
}

}  // namespace codeswitch
