// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "asm.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "test/test.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

// For each .csws file in the testdata, assemble the file, disassemble it,
// then assemble it again. Check that the bytecode is identical from both
// assembly passes. The disassembly will be a bit different, since it won't
// include comments or handwritten labels.
TEST(AssembleDisassemble) {
  filesystem::path path("package/testdata");
  for (filesystem::directory_iterator it(path); it != filesystem::directory_iterator(); it++) {
    auto filename = it->path();
    if (filename.extension() != ".csws") {
      continue;
    }
    std::ifstream file(filename);
    auto package1 = readPackageAsm(filename, file);
    std::stringstream dis;
    writePackageAsm(dis, package1.get());
    auto package2 = readPackageAsm(filename, dis);

    ASSERT_EQ(package1->functions().length(), package2->functions().length());
    for (length_t i = 0, n = package1->functions().length(); i < n; i++) {
      auto f1 = package1->functions()[i];
      auto f2 = package2->functions()[i];
      ASSERT_EQ(f1->name(), f2->name());
      ASSERT_EQ(f1->insts().length(), f2->insts().length());
      auto f1begin = reinterpret_cast<const uint8_t*>(&*f1->insts().begin());
      auto f1end = f1begin + f1->insts().length();
      auto f2begin = reinterpret_cast<const uint8_t*>(&*f2->insts().begin());
      ASSERT_TRUE(std::equal(f1begin, f1end, f2begin));
    }
  }
}

}  // namespace codeswitch