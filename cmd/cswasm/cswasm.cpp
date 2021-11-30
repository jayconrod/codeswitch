// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include <exception>
#include <fstream>
#include <iostream>
#include "common/error.h"
#include "flag/flag.h"
#include "package/asm.h"

int main(int argc, char* argv[]) {
  try {
    codeswitch::FlagSet flags(argv[0], "-o=out.cswp in.csws");
    std::string outPath;
    flags.stringFlag(&outPath, "o", "", "name of CodeSwitch package file to write", true);
    auto argStart = flags.parse(argc - 1, argv + 1);
    if (argStart != static_cast<size_t>(argc - 2)) {
      throw codeswitch::errorf("expected 1 position argument; got %d", argc - 1 - argStart);
    }
    std::string inPath(argv[argc - 1]);

    std::ifstream inFile(inPath);
    auto package = codeswitch::readPackageAsm(inPath, inFile);
    inFile.close();

    package->writeToFile(outPath);
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
  }
}
