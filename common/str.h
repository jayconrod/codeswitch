// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_str_h
#define common_str_h

#include <cstdarg>
#include <sstream>
#include <string>

namespace codeswitch {

// TODO: remove these, migrate callers to buildString. These are almost always
// used incorrectly. It's hard to get the type specifiers right.
std::string strprintf(const char* fmt, ...);
std::string vstrprintf(const char* fmt, va_list ap);

inline std::string buildString_(std::stringstream* buf) {
  return buf->str();
}

template <class T, class... Args>
std::string buildString_(std::stringstream* buf, T arg, Args... args) {
  *buf << arg;
  return buildString_(buf, args...);
}

template <class... Args>
std::string buildString(Args... args) {
  std::stringstream buf;
  return buildString_(&buf, args...);
}

}  // namespace codeswitch

#endif
