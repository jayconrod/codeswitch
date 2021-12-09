// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "bitmap.h"

namespace codeswitch {

uintptr_t Bitmap::sizeFor(uintptr_t bitCount) {
  uintptr_t bits = align(bitCount, kBitsInWord);
  uintptr_t words = bits / kBitsInWord;
  uintptr_t size = words * kWordSize;
  return size;
}

uintptr_t Bitmap::wordCount() const {
  return align(bitCount_, kBitsInWord) / kBitsInWord;
}

bool Bitmap::at(uintptr_t index) const {
  uintptr_t wordIndex = wordIndexForBit(index);
  uintptr_t bitIndex = bitIndexForBit(index);
  bool value = (base_[wordIndex] & (1ULL << bitIndex)) != 0;
  return value;
}

uintptr_t Bitmap::wordAt(uintptr_t wordIndex) const {
  ASSERT(wordIndex * kBitsInWord < bitCount_);
  return base_[wordIndex];
}

void Bitmap::set(uintptr_t index, bool value) {
  uintptr_t wordIndex = wordIndexForBit(index);
  uintptr_t bitIndex = bitIndexForBit(index);
  uintptr_t* wp = base_ + wordIndex;
  *wp = bitInsert(*wp, static_cast<uintptr_t>(value), 1, bitIndex);
}

void Bitmap::setWord(uintptr_t wordIndex, uintptr_t value) {
  ASSERT(wordIndex * kBitsInWord < bitCount_);
  base_[wordIndex] = value;
}

void Bitmap::clear() {
  for (uintptr_t i = 0, n = wordCount(); i < n; i++) base_[i] = 0;
}

void Bitmap::copyFrom(Bitmap bitmap) {
  ASSERT(bitCount_ == bitmap.bitCount_);
  for (uintptr_t i = 0, n = wordCount(); i < n; i++) base_[i] = bitmap.base_[i];
}

uintptr_t Bitmap::wordIndexForBit(uintptr_t index) const {
  ASSERT(index < bitCount_);
  return index / kBitsInWord;
}

uintptr_t Bitmap::bitIndexForBit(uintptr_t index) const {
  ASSERT(index < bitCount_);
  return index % kBitsInWord;
}

}  // namespace codeswitch
