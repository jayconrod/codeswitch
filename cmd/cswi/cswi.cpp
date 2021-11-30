// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include <exception>
#include <iostream>
#include "common/error.h"
#include "flag/flag.h"
#include "interpreter/interpreter.h"
#include "memory/handle.h"
#include "package/package.h"

int main(int argc, char* argv[]) {
  try {
    codeswitch::FlagSet flags(argv[0], "in.cswp");
    bool validate;
    flags.boolFlag(&validate, "v", false, "validate all packages before interpreting anything");
    auto argStart = flags.parse(argc - 1, argv + 1);
    if (argStart != static_cast<size_t>(argc - 2)) {
      throw codeswitch::errorstr("expected 1 positional argument; got ", argc - 1 - argStart);
    }
    std::string inPath(argv[argc - 1]);

    auto package = codeswitch::Package::readFromFile(inPath);
    if (validate) {
      package->validate();
    }
    auto entryName = codeswitch::String::create("main");
    auto entryFn = handle(package->functionByName(**entryName));
    if (!entryFn) {
      throw codeswitch::errorstr(inPath, ": could not function entry function 'main'");
    }

    codeswitch::interpret(package, entryFn);
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
