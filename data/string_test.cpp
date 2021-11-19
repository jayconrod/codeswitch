// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "string.h"

namespace codeswitch {

TEST(StringCompare) {
  auto a = String::create("foo");
  ASSERT_EQ(a->compare(*a.get()), 0);
  ASSERT_EQ(a->compare("foo"), 0);
  auto b = String::create(std::string("foo"));
  ASSERT_EQ(a->compare(*b.get()), 0);
  auto c = String::create("bar");
  ASSERT_TRUE(a->compare(*c.get()) > 0);
  ASSERT_TRUE(c->compare(*a.get()) < 0);
  ASSERT_TRUE(a->compare("bar") > 0);
  *b.get() = *a.get();
  b->slice(0, 2);
  ASSERT_TRUE(a->compare(*b.get()) > 0);
}

TEST(StringSlice) {
  auto a = String::create("abcde");
  auto s = handle(String::make());

  *s.get() = *a.get();
  s->slice(0, 0);
  ASSERT_EQ(s->compare(""), 0);

  *s.get() = *a.get();
  s->slice(2, 2);
  ASSERT_EQ(s->compare(""), 0);

  *s.get() = *a.get();
  s->slice(5, 5);
  ASSERT_EQ(s->compare(""), 0);

  *s.get() = *a.get();
  s->slice(0, 2);
  ASSERT_EQ(s->compare("ab"), 0);

  *s.get() = *a.get();
  s->slice(2, 5);
  ASSERT_EQ(s->compare("cde"), 0);

  *s.get() = *a.get();
  try {
    s->slice(0, 6);
  } catch (BoundsCheckError& err) {
    return;
  }
  t.fatal("String::slice did not perform bounds check");
}

}  // namespace codeswitch
