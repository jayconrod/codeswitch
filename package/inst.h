// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_inst_h
#define package_inst_h

#include <cstdint>
#include "common/common.h"

namespace codeswitch {

enum class Op : uint8_t {
  RET,
  UNIT,
  INT64,
};

class Inst {
 public:
  Op op;

  inline word_t size() const;
  inline const Inst* next() const;
};

static_assert(sizeof(Inst) == 1);

word_t Inst::size() const {
  switch (op) {
    case Op::RET:
    case Op::UNIT:
      return 1;
    case Op::INT64:
      return 9;
  }
}

const Inst* Inst::next() const {
  return this + size();
}

}  // namespace codeswitch

#endif
