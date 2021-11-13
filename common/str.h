// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_str_h
#define common_str_h

#include <cstdarg>
#include <string>

namespace codeswitch {

std::string strprintf(const char* fmt, ...);
std::string vstrprintf(const char* fmt, va_list ap);

}  // namespace codeswitch

#endif
