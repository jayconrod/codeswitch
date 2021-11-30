// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_type_h
#define package_type_h

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
  static Type* make(Kind kind) { return new (heap.allocate(sizeof(Type))) Type(kind); }

  Kind kind() const { return kind_; }
  word_t size() const;
  bool operator==(const Type& other) const;
  bool operator!=(const Type& other) const { return !(*this == other); }
  word_t hash() const;

 private:
  Kind kind_ = Kind::UNIT;
};

}  // namespace codeswitch

#endif
