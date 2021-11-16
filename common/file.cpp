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

std::vector<uint8_t> readAll(std::istream& is) {
  if (!is.good()) {
    throw FileError("<unknown>", "unknown I/O error");
  }
  auto pos = is.tellg();
  is.seekg(0, std::ios::end);
  if (!is.good()) {
    throw FileError("<unknown>", "could not get file size");
  }
  auto size = is.tellg() - pos;
  is.seekg(pos, std::ios::beg);
  std::vector<uint8_t> data(size);
  is.read(reinterpret_cast<char*>(data.data()), data.size());
  if (!is.good()) {
    throw FileError("<unknown>", "could not read file");
  }
  return data;
}

std::vector<uint8_t> readFile(const filesystem::path& filename) {
  std::ifstream f(filename);
  if (!f.good()) {
    throw FileError(filename, "could not open file");
  }
  try {
    return readAll(f);
  } catch (FileError& err) {
    err.path = filename;
    throw err;
  }
}

FileError::FileError(const filesystem::path& path, const std::string& message) :
    Error(strprintf("%s: %s", path.c_str(), message.c_str())), path(path), message(message) {}

}  // namespace codeswitch
