// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "asm.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "platform/platform.h"
#include "test/test.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

static void checkPackagesEqual(Test& t, Handle<Package>& p1, Handle<Package>& p2);

// For each .csws file in testdata, assemble the file, disassemble it,
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
    package1->validate();
    std::stringstream dis;
    writePackageAsm(dis, *package1);
    auto package2 = readPackageAsm(filename, dis);
    package2->validate();
    checkPackagesEqual(t, package1, package2);
  }
}

// For each .csws file in testdata, assemble the file, write it to a binary
// temporary file, then read it back in. Check that the bytecode is identical
// from both reads.
TEST(SerializeDeserialize) {
  filesystem::path path("package/testdata");
  for (filesystem::directory_iterator it(path); it != filesystem::directory_iterator(); it++) {
    auto filename = it->path();
    if (filename.extension() != ".csws") {
      continue;
    }
    std::ifstream file(filename);
    auto package1 = readPackageAsm(filename, file);
    package1->validate();
    TempFile tmp(filename.stem().string() + "-*.cswp");
    package1->writeToFile(tmp.filename);
    std::ifstream tmpFile(tmp.filename);
    auto package2 = Package::readFromFile(tmp.filename);
    package2->validate();
    checkPackagesEqual(t, package1, package2);
  }
}

void checkPackagesEqual(Test& t, Handle<Package>& p1, Handle<Package>& p2) {
  ASSERT_EQ(p1->functionCount(), p2->functionCount());
  for (length_t i = 0, n = p1->functionCount(); i < n; i++) {
    auto f1 = handle(p1->functionByIndex(i));
    auto f2 = handle(p2->functionByIndex(i));
    ASSERT_EQ(f1->name, f2->name);
    ASSERT_EQ(f1->insts.length(), f2->insts.length());
    auto f1begin = reinterpret_cast<const uint8_t*>(f1->insts.begin());
    auto f1end = f1begin + f1->insts.length();
    auto f2begin = reinterpret_cast<const uint8_t*>(f2->insts.begin());
    ASSERT_TRUE(std::equal(f1begin, f1end, f2begin));
  }
}

}  // namespace codeswitch