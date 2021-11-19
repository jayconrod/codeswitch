// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "serial.h"

#include <filesystem>
#include <iostream>
#include "common/common.h"
#include "package.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

const uint8_t kMagic[] = {'C', 'S', 'W', 'P'};

struct FileHeader {
  uint8_t magic[4];
  uint8_t version;
  uint8_t endiannessAndByteSize;
  uint16_t sectionCount;
};

namespace SectionKind {
const uint32_t FUNCTION = 1;
const uint32_t TYPE = 2;
const uint32_t STRING = 3;
}  // namespace SectionKind

struct SectionHeader {
  uint32_t kind;
  uint32_t count;
  uint64_t offset;
  uint64_t dataOffset;
};

Handle<Package> readPackageBinary(const std::filesystem::path& filename, std::istream& is) {
  UNIMPLEMENTED();
  return Handle<Package>();
}

void writePackageBinary(std::ostream& os, const Package* package) {
  UNIMPLEMENTED();
}

}  // namespace codeswitch
