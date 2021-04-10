// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_str_h
#define common_str_h

#include <string>

namespace codeswitch {
namespace internal {

std::string strprintf(const char* fmt, ...);

}  // namespace internal
}  // namespace codeswitch

#endif
