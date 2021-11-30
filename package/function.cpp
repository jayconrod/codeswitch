// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "function.h"

#include <algorithm>
#include <vector>
#include "common/common.h"
#include "inst.h"
#include "memory/handle.h"
#include "package.h"
#include "roots.h"
#include "type.h"

namespace codeswitch {

struct ValidationBlock {
  std::vector<Type*> types;
  length_t begin = 0, end = 0;
  bool live = false;

  static bool less(const ValidationBlock& l, const ValidationBlock& r) { return l.begin < r.begin; }
};

void Function::validate(Handle<Package>& package) {
  // TODO: validate frame size
  std::vector<ValidationBlock> blocks;
  std::vector<int> blockStack;
  blocks.emplace_back(ValidationBlock{.live = true});
  blockStack.push_back(0);

  auto checkBranch = [this, &blocks, &blockStack](const Inst* inst, int32_t rel, std::vector<Type*>&& types) {
    int32_t instOffset = inst - insts.begin();
    if (addWouldOverflow(rel, instOffset) || instOffset + rel < 0 ||
        static_cast<length_t>(instOffset + rel) >= insts.length()) {
      throw ValidateError("", name.str(),
                          buildString("at offset ", instOffset, ", instruction ", inst->mnemonic(),
                                      " has target offset ", rel, " out of range"));
    }
    auto targetOffset = static_cast<length_t>(instOffset + rel);
    ValidationBlock b{.begin = targetOffset};
    auto it = std::lower_bound(blocks.begin(), blocks.end(), b, ValidationBlock::less);
    if (it == blocks.end() || it->begin != targetOffset) {
      // branch to new block
      b.types = types;
      b.live = true;
      it = blocks.emplace(it, b);
    } else {
      // branch to known block
      if (it->types.size() != types.size()) {
        throw ValidateError(
            "", name.str(),
            buildString("at offset ", instOffset, ", branch to block at ", targetOffset, " with stack depth ",
                        types.size(), " but another branch to the same block has stack depth ", it->types.size()));
      }
      for (size_t i = 0, n = it->types.size(); i < n; i++) {
        if (*it->types[i] != *types[i]) {
          throw ValidateError("", name.str(),
                              buildString("at offset ", instOffset, ", branch to block at ", targetOffset,
                                          " with type ", *types[i], " in stack slot ", types.size() - i - 1,
                                          " but another branch to the same block has type ", *it->types[i]));
        }
      }
    }
    if (it->end == 0) {
      blockStack.push_back(it - blocks.begin());
    }
  };

  auto checkType = [this](const Inst* inst, const std::vector<Type*>& types, Type* want, int i, size_t nops) {
    if (types.size() < nops) {
      throw ValidateError("", name.str(),
                          buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction needs ",
                                      nops, " operand(s) on the stack"));
    }
    auto got = types[types.size() - i - 1];
    if (*got != *want) {
      throw ValidateError(
          "", name.str(),
          buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction expects operand ", i,
                      " to have type ", *want, " but found ", *got));
    }
  };

  while (!blockStack.empty()) {
    auto index = blockStack.back();
    blockStack.pop_back();
    auto block = blocks[index];
    if (block.end > 0) {
      continue;
    }

    auto types = block.types;
    auto done = false;
    for (auto inst = insts.begin() + block.begin; !done && inst != insts.end(); inst = inst->next()) {
      if (inst->size() > static_cast<length_t>(insts.end() - inst)) {
        throw ValidateError("", name.str(), buildString("at offset ", inst - insts.begin(), ", truncated instruction"));
      }
      switch (inst->op) {
        case Op::ADD:
        case Op::ASR:
        case Op::DIV:
        case Op::MOD:
        case Op::MUL:
        case Op::SHL:
        case Op::SHR:
        case Op::SUB: {
          checkType(inst, types, roots->int64Type, 0, 2);
          checkType(inst, types, roots->int64Type, 1, 2);
          types.pop_back();
          break;
        }

        case Op::AND:
        case Op::OR:
        case Op::XOR: {
          auto want = roots->int64Type;
          if (!types.empty() && types.back()->kind() == Type::Kind::BOOL) {
            want = roots->boolType;
          }
          checkType(inst, types, want, 0, 2);
          checkType(inst, types, want, 1, 2);
          types.pop_back();
          break;
        }

        case Op::B: {
          blocks[index].end = inst->next() - insts.begin();
          auto rel = *reinterpret_cast<const int32_t*>(inst + 1);
          checkBranch(inst, rel, std::move(types));
          done = true;
          break;
        }

        case Op::BIF: {
          checkType(inst, types, roots->boolType, 0, 1);
          types.pop_back();
          blocks[index].end = inst->next() - insts.begin();
          auto rel = *reinterpret_cast<const int32_t*>(inst + 1);
          auto types2 = types;
          checkBranch(inst, rel, std::move(types));
          checkBranch(inst, inst->size(), std::move(types2));
          done = true;
          break;
        }

        case Op::CALL: {
          auto functionIndex = *reinterpret_cast<const uint32_t*>(inst + 1);
          if (functionIndex >= package->functionCount()) {
            throw ValidateError("", name.str(),
                                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(),
                                            " instruction has invalid function index ", functionIndex));
          }
          auto callee = package->functionByIndex(functionIndex);
          for (length_t i = 0, n = callee->paramTypes.length(); i < n; i++) {
            checkType(inst, types, callee->paramTypes[i].get(), n - i - 1, n);
          }
          types.erase(types.end() - callee->paramTypes.length(), types.end());
          for (auto& t : callee->returnTypes) {
            types.emplace_back(t.get());
          }
          break;
        }

        case Op::EQ:
        case Op::NE: {
          if (types.size() < 2) {
            throw ValidateError("", name.str(),
                                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(),
                                            " needs 2 operands on the stack"));
          }
          auto r = types[types.size() - 1];
          auto l = types[types.size() - 2];
          if (*l != *r) {
            throw ValidateError(
                "", name.str(),
                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(),
                            " instruction requires two operands of the same type; got ", l, " and ", r));
          }
          types.pop_back();
          types.pop_back();
          types.push_back(roots->boolType);
          break;
        }

        case Op::FALSE:
        case Op::TRUE:
          types.push_back(roots->boolType);
          break;

        case Op::GE:
        case Op::GT:
        case Op::LE:
        case Op::LT: {
          checkType(inst, types, roots->int64Type, 0, 2);
          checkType(inst, types, roots->int64Type, 0, 2);
          types.pop_back();
          types.pop_back();
          types.push_back(roots->boolType);
          break;
        }

        case Op::INT64:
          types.push_back(roots->int64Type);
          break;

        case Op::LOADARG: {
          auto index = *reinterpret_cast<const uint16_t*>(inst + 1);
          if (index >= paramTypes.length()) {
            throw ValidateError(
                "", name.str(),
                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction loads argument ",
                            index, " but there are ", paramTypes.length(), " parameter(s)"));
          }
          types.push_back(paramTypes[index].get());
          break;
        }

        case Op::LOADLOCAL: {
          auto index = *reinterpret_cast<const uint16_t*>(inst + 1);
          if (index >= types.size()) {
            throw ValidateError(
                "", name.str(),
                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction loads local ",
                            index, " but there are ", types.size(), " locals"));
          }
          types.push_back(types[index]);
          break;
        }

        case Op::NEG: {
          checkType(inst, types, roots->int64Type, 0, 1);
          break;
        }

        case Op::NOP:
          break;

        case Op::NOT: {
          auto want = roots->boolType;
          if (!types.empty() && types.back()->kind() == Type::Kind::INT64) {
            want = types.back();
          }
          checkType(inst, types, want, 0, 1);
          break;
        }

        case Op::RET: {
          for (length_t i = 0, n = returnTypes.length(); i < n; i++) {
            checkType(inst, types, returnTypes[i].get(), n - i - 1, n);
          }
          blocks[index].end = inst->next() - insts.begin();
          done = true;
          break;
        }

        case Op::STOREARG: {
          auto index = *reinterpret_cast<const uint16_t*>(inst + 1);
          if (types.empty()) {
            throw ValidateError("", name.str(),
                                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(),
                                            " instruction with empty stack"));
          }
          auto type = types.back();
          types.pop_back();
          if (index >= paramTypes.length()) {
            throw ValidateError(
                "", name.str(),
                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction stores argument ",
                            index, " but there are ", paramTypes.length(), " parameter(s)"));
          }
          if (*paramTypes[index] != *type) {
            throw ValidateError(
                "", name.str(),
                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction stores argument ",
                            index, " with type ", *paramTypes[index], " but operand has type ", *type));
          }
          break;
        }

        case Op::STORELOCAL: {
          auto index = *reinterpret_cast<const uint16_t*>(inst + 1);
          if (types.empty()) {
            throw ValidateError("", name.str(),
                                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(),
                                            " instruction with empty stack"));
          }
          if (index >= types.size() - 1) {
            throw ValidateError(
                "", name.str(),
                buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(), " instruction stores local ",
                            index, " but there are ", types.size() - 1, " locals"));
          }
          auto type = types.back();
          types.pop_back();
          types[index] = type;
          break;
        }

        case Op::SYS: {
          auto sys = *reinterpret_cast<const Sys*>(inst + 1);
          switch (sys) {
            case Sys::EXIT:
              checkType(inst, types, roots->int64Type, 0, 1);
              break;
            case Sys::PRINTLN:
              checkType(inst, types, roots->int64Type, 0, 1);
              types.pop_back();
              break;
            default:
              throw ValidateError("", name.str(),
                                  buildString("at offset ", inst - insts.begin(), ", ", inst->mnemonic(),
                                              " instruction with unknown system function"));
          }
          break;
        }

        case Op::UNIT:
          types.push_back(roots->unitType);

        default:
          throw ValidateError("", name.str(), buildString("unknown opcode at offset ", inst - insts.begin()));
      }
    }
  }

  // Make sure there's no dead space inside the function.
  length_t prevEnd = 0;
  for (auto& b : blocks) {
    if (b.begin != prevEnd) {
      throw ValidateError(
          "", name.str(),
          buildString("block starting at ", b.begin, " does not start immediately after previous block"));
    }
    prevEnd = b.end;
  }
}

}  // namespace codeswitch
