// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "common.h"

namespace codeswitch {

TEST(Alignment) {
  ASSERT_EQ(0u, align(0, 4));
  ASSERT_EQ(4u, align(1, 4));
  ASSERT_EQ(4u, align(3, 4));
  ASSERT_TRUE(isAligned(0, 4));
  ASSERT_FALSE(isAligned(1, 4));
  ASSERT_FALSE(isAligned(3, 4));
  ASSERT_TRUE(isAligned(4, 4));
}

TEST(Bits) {
  ASSERT_TRUE(bit(0x10, 4));
  ASSERT_FALSE(bit(0x10, 3));

  ASSERT_EQ(0x33u, bitExtract(0xF33F00, 8, 12));
  ASSERT_EQ(0xFFFF00u, bitInsert(0xF33F00, 0xFF, 8, 12));
}

TEST(IsPowerOf2) {
  ASSERT_FALSE(isPowerOf2(0));
  ASSERT_TRUE(isPowerOf2(1));
  ASSERT_TRUE(isPowerOf2(2));
  ASSERT_FALSE(isPowerOf2(3));
  ASSERT_TRUE(isPowerOf2(static_cast<word_t>(1) << static_cast<word_t>(31)));
}

TEST(NextPowerOf2) {
  ASSERT_EQ(nextPowerOf2(0), static_cast<word_t>(1));
  ASSERT_EQ(nextPowerOf2(1), static_cast<word_t>(1));
  ASSERT_EQ(nextPowerOf2(2), static_cast<word_t>(2));
  ASSERT_EQ(nextPowerOf2(3), static_cast<word_t>(4));
  ASSERT_EQ(nextPowerOf2(4), static_cast<word_t>(4));
  ASSERT_EQ(nextPowerOf2(5), static_cast<word_t>(8));
}

}  // namespace codeswitch
