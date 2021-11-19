// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_serial_h
#define package_serial_h

#include <filesystem>
#include <iostream>

namespace codeswitch {

template <class T>
class Handle;
class Package;

Handle<Package> readPackageBinary(const std::filesystem::path& filename, std::istream& is);
void writePackageBinary(std::ostream& os, const Package* package);

}  // namespace codeswitch

#endif
