// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef bitmap_h
#define bitmap_h

#include "common/common.h"

namespace codeswitch {
namespace internal {

class Bitmap {
 public:
  Bitmap(word_t* base, word_t bitCount) : base_(base), bitCount_(bitCount) {}

  static word_t sizeFor(word_t bitCount);

  word_t* base() { return base_; }
  word_t bitCount() const { return bitCount_; }
  word_t wordCount() const;
  bool at(word_t index) const;
  bool operator[](word_t index) const { return at(index); }
  word_t wordAt(word_t wordIndex) const;
  void set(word_t index, bool value);
  void setWord(word_t wordIndex, word_t value);
  void clear();

  void copyFrom(Bitmap bitmap);

 private:
  word_t wordIndexForBit(word_t index) const;
  word_t bitIndexForBit(word_t index) const;

  word_t* base_;
  word_t bitCount_;
};
}  // namespace internal
}  // namespace codeswitch

#endif
