// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "interpreter.h"

#include <cstring>
#include <iostream>
#include "common/common.h"
#include "memory/handle.h"
#include "memory/stack.h"
#include "package/function.h"
#include "package/package.h"

namespace codeswitch {

static length_t typesSize(const List<Ptr<Type>>& types);

void interpret(Handle<Package>& package, Handle<Function>& entry, std::ostream& out) {
  // TODO: allow the entry function to have parameters and return values.
  // There must be a way to pass values between native and interpreted code.
  ASSERT(entry->returnTypes().length() == 0);
  ASSERT(entry->paramTypes().length() == 0);

  // Create the stack. We'll keep sp and fp as local variables for speed,
  // but we need to save them back to s.sp and s.fp before doing anything
  // outside this function.
  Stack s;
  auto fp = reinterpret_cast<Frame*>(s.fp) - 1;
  auto sp = reinterpret_cast<word_t*>(fp);

#define CHECK_STACK(size)                                   \
  do {                                                      \
    if (reinterpret_cast<address>(sp) - size < s.limit()) { \
      s.sp = reinterpret_cast<address>(sp);                 \
      s.fp = reinterpret_cast<address>(fp);                 \
      s.check(size);                                        \
    }                                                       \
  } while (false)

#define PUSH(x) *--sp = x

#define POP() *sp++

#define UNARY_OP(op)                      \
  do {                                    \
    auto x = static_cast<int64_t>(POP()); \
    PUSH(op x);                           \
  } while (false);

#define BINARY_OP(op)                     \
  do {                                    \
    auto y = static_cast<int64_t>(POP()); \
    auto x = static_cast<int64_t>(POP()); \
    PUSH(x op y);                         \
  } while (false)

  // Create the initial stack frame.
  *fp = Frame{.fp = 0, .ip = 0, .fn = entry.get(), .pp = package.get()};

  // Set other registers.
  auto fn = entry.get();
  auto ip = &fn->insts().at(0);
  auto pp = package.get();

  while (true) {
    switch (ip->op) {
      case Op::ADD:
        BINARY_OP(+);
        break;

      case Op::AND:
        BINARY_OP(&);
        break;

      case Op::ASR:
        BINARY_OP(>>);
        break;

      case Op::B: {
        auto offset = *reinterpret_cast<const int32_t*>(ip + 1);
        ip += offset;
        continue;
      }

      case Op::BIF: {
        auto cond = static_cast<bool>(POP());
        if (cond) {
          auto offset = *reinterpret_cast<const int32_t*>(ip + 1);
          ip += offset;
          continue;
        }
        break;
      }

      case Op::CALL: {
        auto index = *reinterpret_cast<const uint32_t*>(ip + 1);
        auto frame = reinterpret_cast<Frame*>(sp) - 1;
        *frame = Frame{.fp = fp, .ip = ip->next(), .fn = fn, .pp = pp};
        fn = pp->functions()->at(index).get();
        fp = frame;
        sp = reinterpret_cast<word_t*>(fp);
        ip = &fn->insts()[0];
        CHECK_STACK(fn->frameSize());
        continue;
      }

      case Op::DIV:
        BINARY_OP(/);
        break;

      case Op::FALSE:
        PUSH(0);
        break;

      case Op::EQ:
        BINARY_OP(==);
        break;

      case Op::GE:
        BINARY_OP(>=);
        break;

      case Op::GT:
        BINARY_OP(>);
        break;

      case Op::INT64: {
        auto n = *reinterpret_cast<const word_t*>(ip + 1);
        PUSH(n);
        break;
      }

      case Op::LE:
        BINARY_OP(<=);
        break;

      case Op::LOADARG: {
        auto i = *reinterpret_cast<const uint16_t*>(ip + 1);
        auto ap = reinterpret_cast<word_t*>(fp + 1);
        auto x = ap[i];
        PUSH(x);
        break;
      }

      case Op::LOADLOCAL: {
        auto i = *reinterpret_cast<const int16_t*>(ip + 1);
        auto lp = reinterpret_cast<word_t*>(fp) - 1;
        auto x = lp[-i];
        PUSH(x);
        break;
      }

      case Op::LT:
        BINARY_OP(<);
        break;

      case Op::MOD:
        BINARY_OP(%);
        break;

      case Op::MUL:
        BINARY_OP(*);
        break;

      case Op::NE:
        BINARY_OP(!=);
        break;

      case Op::NEG:
        UNARY_OP(-);
        break;

      case Op::NOP:
        break;

      case Op::NOT:
        UNARY_OP(~);
        break;

      case Op::OR:
        BINARY_OP(|);
        break;

      case Op::RET: {
        if (fp->ip == nullptr) {
          return;
        }
        ip = fp->ip;
        auto caller = fp->fn;
        pp = fp->pp;
        auto callerfp = fp->fp;
        auto args = reinterpret_cast<word_t*>(fp + 1);
        auto argWords = typesSize(fn->paramTypes()) / kWordSize;
        auto returnWords = typesSize(fn->returnTypes()) / kWordSize;
        auto src = sp + returnWords;
        auto dst = args + argWords;
        sp = dst - returnWords;
        while (dst != sp) {
          *--dst = *--src;
        }
        fn = caller;
        fp = callerfp;
        continue;
      }

      case Op::SHL:
        BINARY_OP(<<);
        break;

      case Op::SHR:
        BINARY_OP(>>);

      case Op::STOREARG: {
        auto i = *reinterpret_cast<const uint16_t*>(ip + 1);
        auto x = POP();
        auto ap = reinterpret_cast<word_t*>(fp + 1);
        ap[i] = x;
        break;
      }

      case Op::STORELOCAL: {
        auto i = *reinterpret_cast<const int16_t*>(ip + 1);
        auto x = POP();
        auto lp = reinterpret_cast<word_t*>(fp) - 1;
        lp[-i] = x;
        break;
      }

      case Op::SUB:
        BINARY_OP(-);
        break;

      case Op::SYS: {
        auto sys = *reinterpret_cast<const Sys*>(ip + 1);
        switch (sys) {
          case Sys::EXIT: {
            auto status = POP();
            exit(status);
            break;
          }
          case Sys::PRINTLN: {
            auto value = POP();
            out << value << std::endl;
            break;
          }
        }
        break;
      }

      case Op::TRUE:
        PUSH(1);
        break;

      case Op::UNIT:
        PUSH(0);
        break;

      case Op::XOR:
        BINARY_OP(^);
        break;
    }
    ip = ip->next();
  }

#undef BINARY_OP
#undef UNARY_OP
#undef POP
#undef PUSH
#undef CHECK_STACK
}

length_t typesSize(const List<Ptr<Type>>& types) {
  length_t size = 0;
  for (auto& type : types) {
    size += align(type->size(), kWordSize);
  }
  return size;
}

}  // namespace codeswitch
