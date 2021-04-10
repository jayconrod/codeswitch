// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "string.h"

namespace codeswitch {

TEST(StringCompare) {
  auto a = String::make("foo");
  ASSERT_EQ(a->compare(a), 0);
  ASSERT_EQ(a->compare("foo"), 0);
  auto b = String::make(std::string("foo"));
  ASSERT_EQ(a->compare(b), 0);
  auto c = String::make("bar");
  ASSERT_TRUE(a->compare(c) > 0);
  ASSERT_TRUE(c->compare(a) < 0);
  ASSERT_TRUE(a->compare("bar") > 0);
  ASSERT_TRUE(a->compare(a->slice(0, 2)) > 0);
}

TEST(StringSlice) {
  auto a = String::make("abcde");
  ASSERT_EQ(a->slice(0, 0).compare(""), 0);
  ASSERT_EQ(a->slice(2, 2).compare(""), 0);
  ASSERT_EQ(a->slice(5, 5).compare(""), 0);
  ASSERT_EQ(a->slice(0, 2).compare("ab"), 0);
  ASSERT_EQ(a->slice(2, 5).compare("cde"), 0);

  try {
    a->slice(0, 6);
  } catch (BoundsCheckError& err) {
    return;
  }
  t.fatal("String::slice did not perform bounds check");
}

}  // namespace codeswitch
