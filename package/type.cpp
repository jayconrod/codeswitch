// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "type.h"

#include <functional>
#include "common/common.h"

namespace codeswitch {

uintptr_t Type::size() const {
  switch (kind_) {
    case UNIT:
      return 0;
    case BOOL:
      return 1;
    case INT64:
      return 8;
  }
  ABORT("unreachable");
  return ~0;
}

uint16_t Type::stackSlotSize() const {
  return static_cast<uint16_t>(align(size(), kWordSize) / kWordSize);
}

bool Type::operator==(const Type& other) const {
  return kind_ == other.kind_;
}

uintptr_t Type::hash() const {
  return std::hash<int>{}(kind_);
}

std::ostream& operator<<(std::ostream& os, const Type& type) {
  return os << type.kind_;
}

std::ostream& operator<<(std::ostream& os, Type::Kind kind) {
  const char* name;
  switch (kind) {
    case Type::Kind::UNIT:
      name = "unit";
      break;
    case Type::Kind::BOOL:
      name = "bool";
      break;
    case Type::Kind::INT64:
      name = "int64";
      break;
  }
  return os << name;
}

}  // namespace codeswitch
