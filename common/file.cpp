// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "file.h"

#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>
#include "error.h"
#include "str.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

std::vector<uint8_t> readFile(const filesystem::path& filename) {
  std::ifstream f(filename);
  if (!f.good()) {
    throw FileError(filename, "could not open file");
  }
  f.seekg(0, std::ios::end);
  auto size = f.tellg();
  if (!f.good()) {
    throw FileError(filename, "could not get file size");
  }
  f.seekg(0, std::ios::beg);
  std::vector<uint8_t> data(size);
  f.read(reinterpret_cast<char*>(data.data()), data.size());
  if (!f.good()) {
    throw FileError(filename, "could not read file");
  }
  return data;
}

FileError::FileError(const filesystem::path& path, const std::string& message) :
    Error(strprintf("%s: %s", path.c_str(), message.c_str())), path(path), message(message) {}

}  // namespace codeswitch
