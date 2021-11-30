// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include <exception>
#include <fstream>
#include <iostream>
#include "common/error.h"
#include "common/file.h"
#include "flag/flag.h"
#include "package/asm.h"

int main(int argc, char* argv[]) {
  try {
    codeswitch::FlagSet flags(argv[0], "-o=out.cswp in.csws");
    bool disassemble;
    std::string outPath;
    flags.boolFlag(&disassemble, "d", false, "disassemble a binary file instead of assembling a text file");
    flags.stringFlag(&outPath, "o", "", "name of CodeSwitch package file to write",
                     codeswitch::FlagSet::Opt::MANDATORY);
    auto argStart = flags.parse(argc - 1, argv + 1);
    if (argStart != static_cast<size_t>(argc - 2)) {
      throw codeswitch::errorstr("expected 1 positional argument; got ", argc - 1 - argStart);
    }
    std::string inPath(argv[argc - 1]);

    if (disassemble) {
      auto package = codeswitch::Package::readFromFile(inPath);
      std::ofstream outFile(outPath);
      if (!outFile.good()) {
        throw codeswitch::FileError(outPath, "could not write file");
      }
      codeswitch::writePackageAsm(outFile, *package);
    } else {
      std::ifstream inFile(inPath);
      auto package = codeswitch::readPackageAsm(inPath, inFile);
      inFile.close();
      package->writeToFile(outPath);
    }
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
