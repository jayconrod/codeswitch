// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_type_h
#define package_type_h

#include <iostream>
#include "common/common.h"
#include "memory/heap.h"

namespace codeswitch {

class Type {
 public:
  enum Kind {
    UNIT,
    BOOL,
    INT64,
  };

  Type() = default;
  explicit Type(Kind kind) : kind_(kind) {}
  static Type* make(Kind kind) { return new (heap->allocate(sizeof(Type))) Type(kind); }

  Kind kind() const { return kind_; }
  uintptr_t size() const;
  uint16_t stackSlotSize() const;
  bool operator==(const Type& other) const;
  bool operator!=(const Type& other) const { return !(*this == other); }
  uintptr_t hash() const;

 private:
  friend std::ostream& operator<<(std::ostream&, const Type&);
  Kind kind_ = Kind::UNIT;
};

std::ostream& operator<<(std::ostream& os, const Type& type);
std::ostream& operator<<(std::ostream& os, Type::Kind kind);

}  // namespace codeswitch

#endif
