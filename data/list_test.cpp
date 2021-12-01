// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "test/test.h"

#include "list.h"
#include "memory/handle.h"

namespace codeswitch {

TEST(ListBasic) {
  auto list = handle(List<int>::make());
  ASSERT_EQ(list->length(), static_cast<size_t>(0));
  ASSERT_EQ(list->cap(), static_cast<size_t>(0));

  list->reserve(3);
  ASSERT_EQ(list->length(), static_cast<size_t>(0));
  ASSERT_EQ(list->cap(), static_cast<size_t>(3));
  list->append(10);
  list->append(20);
  list->append(30);
  ASSERT_EQ(list->length(), static_cast<size_t>(3));
  ASSERT_EQ(list->cap(), static_cast<size_t>(3));
  ASSERT_EQ(list->at(0), 10);
  ASSERT_EQ(list->at(1), 20);
  ASSERT_EQ(list->at(2), 30);

  list->append(40);
  ASSERT_EQ(list->length(), static_cast<size_t>(4));
  ASSERT_TRUE(list->cap() > static_cast<size_t>(3));
  ASSERT_EQ(list->at(0), 10);
  ASSERT_EQ(list->at(1), 20);
  ASSERT_EQ(list->at(2), 30);
  ASSERT_EQ(list->at(3), 40);

  try {
    list->at(4);
    t.fatal("bounds check not performed");
  } catch (BoundsCheckError&) {
  }
}

}  // namespace codeswitch
