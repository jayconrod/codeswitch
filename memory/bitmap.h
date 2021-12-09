// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_bitmap_h
#define memory_bitmap_h

#include "common/common.h"

namespace codeswitch {

class Bitmap {
 public:
  Bitmap(uintptr_t* base, uintptr_t bitCount) : base_(base), bitCount_(bitCount) {}

  static uintptr_t sizeFor(uintptr_t bitCount);

  uintptr_t* base() { return base_; }
  uintptr_t bitCount() const { return bitCount_; }
  uintptr_t wordCount() const;
  bool at(uintptr_t index) const;
  bool operator[](uintptr_t index) const { return at(index); }
  uintptr_t wordAt(uintptr_t wordIndex) const;
  void set(uintptr_t index, bool value);
  void setWord(uintptr_t wordIndex, uintptr_t value);
  void clear();

  void copyFrom(Bitmap bitmap);

 private:
  uintptr_t wordIndexForBit(uintptr_t index) const;
  uintptr_t bitIndexForBit(uintptr_t index) const;

  uintptr_t* base_;
  uintptr_t bitCount_;
};
}  // namespace codeswitch

#endif
