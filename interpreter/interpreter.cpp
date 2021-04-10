// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "interpreter.h"

#include <cstring>
#include "common/common.h"
#include "memory/handle.h"
#include "memory/stack.h"
#include "package/function.h"

namespace codeswitch {

static length_t typesSize(const List<Ptr<Type>>& types);

void interpret(Handle<Function> f) {
  ASSERT(f->returnTypes().length() == 0);
  ASSERT(f->paramTypes().length() == 0);
  auto ip = &f->insts().at(0);

  Stack s;
  s.fp -= 2 * kWordSize;
  *reinterpret_cast<address*>(s.fp) = 0;
  *reinterpret_cast<address*>(s.fp + kWordSize) = 0;

  while (true) {
    auto sz = ip->size();
    switch (ip->op) {
      case Op::RET:
        auto returnSize = s.fp - s.sp - f->frameSize();
        auto paramSize = typesSize(f->paramTypes());
        auto delta = paramSize - returnSize;
        ip = *reinterpret_cast<Inst**>(s.fp + kWordSize);
        auto sp = s.fp + 2 * kWordSize + delta;
        s.fp = reinterpret_cast<address*>(s.fp)[0];
        memcpy(reinterpret_cast<void*>(sp), reinterpret_cast<void*>(s.sp), returnSize);
        s.sp = sp;
        if (ip == nullptr) {
          return;
        }
        continue;

      case Op::UNIT:
        s.sp -= kWordSize;
        *reinterpret_cast<word_t*>(s.sp) = 0;

      case Op::INT64:
        s.sp -= sizeof(int64_t);
        auto n = *reinterpret_cast<const int64_t*>(ip + 1);
        *reinterpret_cast<int64_t*>(s.sp) = n;
    }
    ip += sz;
  }
}

}  // namespace codeswitch
