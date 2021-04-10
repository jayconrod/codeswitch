// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "map.h"
#include "string.h"

namespace codeswitch {

class HashInt {
 public:
  static word_t hash(int i) { return static_cast<word_t>(i) * 7919 + 6959; }
  static bool equal(int l, int r) { return l == r; }
};

TEST(MapInt) {
  auto m = handle(Map<int, int, HashInt>::make());
  ASSERT_FALSE(m->has(0));
  ASSERT_FALSE(m->has(99));
  ASSERT_EQ(m->length(), static_cast<length_t>(0));
  for (int i = 0; i < 100; i++) {
    auto key = i * 100;
    ASSERT_FALSE(m->has(key));
    m->set(key, i);
    ASSERT_TRUE(m->has(key));
    ASSERT_EQ(m->get(key), i);
  }
  ASSERT_EQ(m->length(), static_cast<length_t>(100));
}

TEST(MapString) {
  auto m = handle(Map<Ptr<String>, Ptr<String>, HashString>::make());
  ASSERT_EQ(m->length(), static_cast<length_t>(0));
  for (int i = 0; i < 100; i++) {
    auto key = handle(String::make(std::to_string(i)));
    ASSERT_FALSE(m->has(*key));
    m->set(*key, *key);
    ASSERT_TRUE(m->has(*key));
    ASSERT_EQ(m->get(*key)->compare(**key), 0);
  }
}

}  // namespace codeswitch
