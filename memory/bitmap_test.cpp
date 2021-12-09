// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "bitmap.h"
#include "common/common.h"

namespace codeswitch {

const int kWordCount = 2;
uintptr_t testData[kWordCount] = {0x12345678, 0x9abcdef0};

TEST(BitmapAccess) {
  Bitmap bitmap(testData, kWordCount * kBitsInWord);
  ASSERT_EQ(testData, bitmap.base());
  ASSERT_EQ(2 * kBitsInWord, bitmap.bitCount());
  for (uintptr_t i = 0; i < kWordCount; i++) {
    for (uintptr_t j = 0; j < kBitsInWord; j++) {
      bool expected = ((testData[i] >> j) & 1) == 1;
      ASSERT_EQ(expected, bitmap.at(i * kBitsInWord + j));
      ASSERT_EQ(expected, bitmap[i * kBitsInWord + j]);
    }
  }
}

TEST(BitmapMutation) {
  Bitmap bitmap(testData, kWordCount * kBitsInWord);
  ASSERT_EQ(false, bitmap[0]);
  bitmap.set(0, true);
  ASSERT_EQ(true, bitmap[0]);
  ASSERT_EQ(true, bitmap[3]);
  bitmap.set(3, false);
  ASSERT_EQ(false, bitmap[3]);
  bitmap.clear();
  for (uintptr_t i = 0; i < kWordCount * kBitsInWord; i++) {
    ASSERT_EQ(false, bitmap[i]);
  }
}

}  // namespace codeswitch
