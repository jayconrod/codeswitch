// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_asm_h
#define package_asm_h

#include <cstdint>
#include <string>
#include "data/array.h"
#include "inst.h"
#include "memory/handle.h"
#include "package.h"

namespace codeswitch {
namespace internal {

Handle<Package> readPackageAsm(const std::string& filename);

class Assembler {
 public:
  Handle<List<Inst>> finish();

  void fals();
  void int64(int64_t n);
  void ret();
  void tru();
  void unit();

 private:
  std::vector<Inst> insts_;
};

}  // namespace internal
}  // namespace codeswitch

#endif
