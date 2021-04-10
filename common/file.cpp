// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "file.h"

#include <cstdint>
#include <exception>
#include <fstream>
#include <vector>

namespace codeswitch {

std::vector<uint8_t> readFile(const std::string& filename) {
  std::ifstream f;
  f.exceptions(std::ios::badbit);
  f.open(filename, std::fstream::in | std::fstream::binary);
  f.seekg(0, std::ios::end);
  std::vector<uint8_t> data(f.tellg());
  f.seekg(0, std::ios::beg);
  f.read(reinterpret_cast<char*>(data.data()), data.size());
  return data;
}

}  // namespace codeswitch
