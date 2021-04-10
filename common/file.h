// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_file_h
#define common_file_h

#include <cstdint>
#include <string>
#include <vector>

namespace codeswitch {

std::vector<uint8_t> readFile(const std::string& filename);

}  // namespace codeswitch

#endif
