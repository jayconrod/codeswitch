// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "array.h"

namespace codeswitch {
namespace internal {

TEST(ArrayBasic) {
  for (length_t i = 0; i < 2; i++) {
    auto a = Array<int>::make(i);
    for (length_t j = 0; j < i; j++) {
      (*a)[j] = j + 1;
    }
    int sum = 0;
    for (length_t j = 0; j < i; j++) {
      sum += (*a)[j];
    }
    ASSERT_EQ(sum, static_cast<int>(i * (i + 1) / 2));
  }
}

TEST(ArraySlice) {
  auto a = Array<int>::make(2);
  auto b = a->slice(0);
  ASSERT_EQ(a, b);
  auto c = a->slice(1);
  ASSERT_EQ(reinterpret_cast<int*>(c), reinterpret_cast<int*>(a) + 1);
  ASSERT_EQ(&c->at(0), &a->at(1));
  auto d = a->slice(2);
  ASSERT_EQ(reinterpret_cast<int*>(d), reinterpret_cast<int*>(a) + 2);
}

}  // namespace internal
}  // namespace codeswitch
