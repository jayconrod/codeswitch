// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef interpreter_interpreter_h
#define interpreter_interpreter_h

#include <iostream>

namespace codeswitch {

template <class T>
class Handle;
class Function;
class Package;

void interpret(Handle<Package>& package, Handle<Function>& entry, std::ostream& out = std::cerr);

}  // namespace codeswitch

#endif
