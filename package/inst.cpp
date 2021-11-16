// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "inst.h"

#include "common/common.h"

namespace codeswitch {

const char* Inst::mnemonic() const {
  switch (op) {
    case Op::NOP:
      return "nop";
    case Op::SYS:
      return "sys";
    case Op::RET:
      return "ret";
    case Op::CALL:
      return "call";
    case Op::B:
      return "b";
    case Op::BIF:
      return "bif";
    case Op::LOADARG:
      return "loadarg";
    case Op::LOADLOCAL:
      return "loadlocal";
    case Op::STOREARG:
      return "storearg";
    case Op::STORELOCAL:
      return "storelocal";
    case Op::UNIT:
      return "unit";
    case Op::TRUE:
      return "true";
    case Op::FALSE:
      return "false";
    case Op::INT64:
      return "int64";
    case Op::NEG:
      return "neg";
    case Op::NOT:
      return "not";
    case Op::ADD:
      return "add";
    case Op::SUB:
      return "sub";
    case Op::MUL:
      return "mul";
    case Op::DIV:
      return "div";
    case Op::MOD:
      return "mod";
    case Op::SHL:
      return "shl";
    case Op::SHR:
      return "shr";
    case Op::ASR:
      return "asr";
    case Op::AND:
      return "and";
    case Op::OR:
      return "or";
    case Op::XOR:
      return "xor";
    case Op::LT:
      return "lt";
    case Op::LE:
      return "le";
    case Op::GT:
      return "gt";
    case Op::GE:
      return "ge";
    case Op::EQ:
      return "eq";
    case Op::NE:
      return "ne";
  }
  UNREACHABLE();
  return "";
}

const char* sysMnemonic(Sys sys) {
  switch (sys) {
    case Sys::EXIT:
      return "exit";
    case Sys::PRINTLN:
      return "println";
  }
  UNREACHABLE();
  return "";
}

}  // namespace codeswitch