// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_inst_h
#define package_inst_h

#include <cstdint>
#include "common/common.h"

namespace codeswitch {

/**
 * Maximum length in bytes of a function's instructions. This ensures positive
 * and negative offsets fit within the function (in branches) in signed 32-bit
 * integers.
 */
const size_t kMaxFunctionSize = 0x7FFFFFFF;

/**
 * Indicates the operation to be performed by an Inst. Each Inst starts with
 * an Op and may have other operands following that.
 *
 * The integer values here end up in serialized bytecode, so changing values
 * invalidates stored bytecode.
 *
 * Keep this list in sync with:
 *
 * - Inst::mnemonic
 * - Inst::size
 * - PackageBuilder::buildFunction
 * - interpret
 * - writeFunction
 */
enum class Op : uint8_t {
  // System
  NOP,
  SYS,

  // Control flow
  RET,
  CALL,
  B,
  BIF,

  // Memory
  LOADARG,
  LOADLOCAL,
  STOREARG,
  STORELOCAL,

  // Constants
  UNIT,
  TRUE,
  FALSE,
  INT64,

  // Math
  NEG,
  NOT,
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  SHL,
  SHR,
  ASR,
  AND,
  OR,
  XOR,
  LT,
  LE,
  GT,
  GE,
  EQ,
  NE,
};

class Inst {
 public:
  Op op;

  const char* mnemonic() const;
  inline uintptr_t size() const;
  inline bool mayAllocate() const;
  const Inst* next() const { return const_cast<Inst*>(this)->next(); }
  Inst* next() { return this + size(); }
};

static_assert(sizeof(Inst) == 1);

uintptr_t Inst::size() const {
  switch (op) {
    case Op::ADD:
    case Op::AND:
    case Op::ASR:
    case Op::DIV:
    case Op::EQ:
    case Op::FALSE:
    case Op::GE:
    case Op::GT:
    case Op::LE:
    case Op::LT:
    case Op::MOD:
    case Op::MUL:
    case Op::NE:
    case Op::NEG:
    case Op::NOP:
    case Op::NOT:
    case Op::OR:
    case Op::RET:
    case Op::SHL:
    case Op::SHR:
    case Op::SUB:
    case Op::TRUE:
    case Op::UNIT:
    case Op::XOR:
      return 1;
    case Op::SYS:
      return 2;
    case Op::LOADARG:
    case Op::LOADLOCAL:
    case Op::STOREARG:
    case Op::STORELOCAL:
      return 3;
    case Op::B:
    case Op::BIF:
    case Op::CALL:
      return 5;
    case Op::INT64:
      return 9;
  }
  UNREACHABLE();
  return 0;
}

bool Inst::mayAllocate() const {
  switch (op) {
    case Op::CALL:
      return true;
    default:
      return false;
  }
}

/**
 * Codes for VM intrinsic functions, representing system calls. Codes are
 * loosely based on Linux amd64 system call numbers.
 */
enum class Sys : uint8_t {
  EXIT = 60,
  PRINTLN = 127,
};

const char* sysMnemonic(Sys sys);

}  // namespace codeswitch

#endif
