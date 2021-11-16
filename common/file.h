// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef common_file_h
#define common_file_h

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "error.h"

namespace codeswitch {

std::vector<uint8_t> readAll(std::istream& is);
std::vector<uint8_t> readFile(const std::filesystem::path& filename);

class FileError : public Error {
 public:
  FileError(const std::filesystem::path& path, const std::string& message);
  std::filesystem::path path;
  std::string message;
};

}  // namespace codeswitch

#endif
