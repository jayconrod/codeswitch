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
  uint16_t frameSize = 0;
  size_t begin = 0, end = 0;
  bool live = false;

  static bool less(const ValidationBlock& l, const ValidationBlock& r) { return l.begin < r.begin; }
};

Handle<Safepoints> Function::buildSafepoints(Handle<Package>& package) {
  SafepointBuilder spb;
  uint16_t maxFrameSize = 0;
  std::vector<ValidationBlock> blocks;
  std::vector<int> blockStack;
  blocks.emplace_back(ValidationBlock{.live = true});
  blockStack.push_back(0);

  auto branch = [this, &blocks, &blockStack](const Inst* inst, int32_t rel, std::vector<Type*>&& types, uint16_t frameSize) {
    int32_t instOffset = inst - insts.begin();
    if (addWouldOverflow(rel, instOffset) || instOffset + rel < 0 || static_cast<size_t>(instOffset + rel) >= insts.length()) {
      throw ValidateError("", name.str(),
                          buildString("at offset ", instOffset, ", instruction ", inst->mnemonic(),
                                      " has target offset ", rel, " out of range"));
    }
    auto targetOffset = static_cast<size_t>(instOffset + rel);
    ValidationBlock b{.frameSize = frameSize, .begin = targetOffset};
    auto it = std::lower_bound(blocks.begin(), blocks.end(), b, ValidationBlock::less);
    if (it == blocks.end() || it->begin != targetOffset) {
      // branch to new block
      b.types = types;
      b.live = true;
      it = blocks.emplace(it, b);
      blockStack.push_back(it - blocks.begin());
    }
  };

  auto incrFrameSize = [this, &maxFrameSize](const Inst* inst, uint16_t* frameSize, int16_t delta) {
    auto instOffset = inst - insts.begin();
    if (addWouldOverflow(static_cast<int16_t>(*frameSize), delta)) {
      throw ValidateError("", name.str(), buildString("at offset ", instOffset, ", instruction ", inst->mnemonic(), " causes frame size to overflow"));
    }
    *frameSize += delta;
    if (*frameSize > maxFrameSize) {
      maxFrameSize = *frameSize;
    }
  };

  auto recordSafepoint = [this, &spb](const Inst* inst, std::vector<Type*>& types) {
    auto instOffset = static_cast<uint32_t>(inst - insts.begin());
    spb.newEntry(instOffset);
    // TODO: when types can be pointers, set pointer bits
  };

  while (!blockStack.empty()) {
    auto index = blockStack.back();
    blockStack.pop_back();
    auto block = blocks[index];
    if (block.end > 0) {
      // already visited
      continue;
    }

    auto types = block.types;
    auto frameSize = block.frameSize;
    auto done = false;
    for (auto inst = insts.begin() + block.begin; !done && inst != insts.end(); inst = inst->next()) {
      switch (inst->op) {
        case Op::ADD:
        case Op::AND:
        case Op::ASR:
        case Op::DIV:
        case Op::MOD:
        case Op::MUL:
        case Op::OR:
        case Op::SHL:
        case Op::SHR:
        case Op::SUB:
        case Op::XOR:
          types.pop_back();
          incrFrameSize(inst, &frameSize, -1);
          break;

        case Op::B: {
          blocks[index].end = inst->next() - insts.begin();
          auto rel = *reinterpret_cast<const int32_t*>(inst + 1);
          branch(inst, rel, std::move(types), frameSize);
          done = true;
          break;
        }
        
        case Op::BIF: {
          blocks[index].end = inst->next() - insts.begin();
          types.pop_back();
          incrFrameSize(inst, &frameSize, -1);
          auto rel = *reinterpret_cast<const int32_t*>(inst + 1);
          auto types2 = types;
          branch(inst, rel, std::move(types), frameSize);
          branch(inst, inst->size(), std::move(types2), frameSize);
          done = true;
          break;
        }

        case Op::CALL: {
          recordSafepoint(inst + inst->size(), types);
          auto functionIndex = *reinterpret_cast<const uint32_t*>(inst + 1);
          auto callee = package->functionByIndex(functionIndex);
          int16_t frameSizeDelta = 0;
          for (size_t i = 0, n = callee->paramTypes.length(); i < n; i++) {
            auto ty = types.back();
            types.pop_back();
            frameSizeDelta -= ty->stackSlotSize();
          }
          for (size_t i = 0, n = callee->returnTypes.length(); i < n; i++) {
            auto ty = callee->returnTypes[i].get();
            types.push_back(ty);
            frameSizeDelta += ty->stackSlotSize();
          }
          incrFrameSize(inst, &frameSize, frameSizeDelta);
          break;
        }

        case Op::EQ:
        case Op::GE:
        case Op::GT:
        case Op::LE:
        case Op::LT:
        case Op::NE:
          types.pop_back();
          types.pop_back();
          types.push_back(roots->boolType);
          incrFrameSize(inst, &frameSize, -1);
          break;

        case Op::FALSE:
        case Op::TRUE:
          types.push_back(roots->boolType);
          incrFrameSize(inst, &frameSize, 1);
          break;
        
        case Op::INT64:
          types.push_back(roots->int64Type);
          incrFrameSize(inst, &frameSize, 1);
          break;
        
        case Op::LOADARG: {
          auto index = *reinterpret_cast<const int16_t*>(inst + 1);
          auto ty = paramTypes[index].get();
          types.push_back(ty);
          incrFrameSize(inst, &frameSize, ty->stackSlotSize());
          break;
        }

        case Op::LOADLOCAL: {
          auto index = *reinterpret_cast<const int16_t*>(inst + 1);
          auto ty = types[index];
          types.push_back(ty);
          incrFrameSize(inst, &frameSize, ty->stackSlotSize());
          break;
        }

        case Op::NEG:
        case Op::NOP:
        case Op::NOT:
          break;

        case Op::RET:
          blocks[index].end = inst->next() - insts.begin();
          done = true;
          break;

        case Op::STOREARG:
        case Op::STORELOCAL: {
          auto ty = types.back();
          types.pop_back();
          incrFrameSize(inst, &frameSize, -static_cast<int32_t>(ty->stackSlotSize()));
          break;
        }

        case Op::SYS: {
          auto sys = *reinterpret_cast<const Sys*>(inst + 1);
          switch (sys) {
            case Sys::EXIT:
              blocks[index].end = inst->next() - insts.begin();
              done = true;
              break;
            
            case Sys::PRINTLN:
              recordSafepoint(inst + inst->size(), types);
              auto ty = types.back();
              types.pop_back();
              incrFrameSize(inst, &frameSize, -static_cast<int32_t>(ty->stackSlotSize()));
              break;
          }
          break;
        }

        case Op::UNIT:
          types.push_back(roots->unitType);
          incrFrameSize(inst, &frameSize, 1);
          break;
      }
    }
  }

  return spb.build(maxFrameSize);
}

void Function::validate(Handle<Package>& package) {
  std::vector<ValidationBlock> blocks;
  std::vector<int> blockStack;
  blocks.emplace_back(ValidationBlock{.live = true});
  blockStack.push_back(0);

  auto checkBranch = [this, &blocks, &blockStack](const Inst* inst, int32_t rel, std::vector<Type*>&& types, uint16_t frameSize) {
    int32_t instOffset = inst - insts.begin();
    if (addWouldOverflow(rel, instOffset) || instOffset + rel < 0 ||
        static_cast<size_t>(instOffset + rel) >= insts.length()) {
      throw ValidateError("", name.str(),
                          buildString("at offset ", instOffset, ", instruction ", inst->mnemonic(),
                                      " has target offset ", rel, " out of range"));
    }
    auto targetOffset = static_cast<size_t>(instOffset + rel);
    ValidationBlock b{.frameSize = frameSize, .begin = targetOffset};
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
            buildString("at offset ", instOffset, ", branch to block at ", targetOffset, " with ",
                        types.size(), " types on stack, but another branch to the same block has ", it->types.size(), " types on stack"));
      }
      if (it->frameSize != frameSize) {
        throw ValidateError(
          "", name.str(),
          buildString("at offset ", instOffset, ", branch to block at ", targetOffset, " with stack depth ",
            frameSize, " but another branch to the same block has stack depth ", it->frameSize));
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
    auto frameSize = block.frameSize;
    auto done = false;
    for (auto inst = insts.begin() + block.begin; !done && inst != insts.end(); inst = inst->next()) {
      if (inst->size() > static_cast<size_t>(insts.end() - inst)) {
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
          checkBranch(inst, rel, std::move(types), frameSize);
          done = true;
          break;
        }

        case Op::BIF: {
          checkType(inst, types, roots->boolType, 0, 1);
          types.pop_back();
          blocks[index].end = inst->next() - insts.begin();
          auto rel = *reinterpret_cast<const int32_t*>(inst + 1);
          auto types2 = types;
          checkBranch(inst, rel, std::move(types), frameSize);
          checkBranch(inst, inst->size(), std::move(types2), frameSize);
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
          for (size_t i = 0, n = callee->paramTypes.length(); i < n; i++) {
            checkType(inst, types, callee->paramTypes[i].get(), n - i - 1, n);
          }
          int16_t frameSizeDelta = 0;
          for (auto it = types.end() - callee->paramTypes.length(); it < types.end(); it++) {
            frameSizeDelta -= align((*it)->size(), kWordSize) / kWordSize;
          }
          types.erase(types.end() - callee->paramTypes.length(), types.end());
          for (auto& t : callee->returnTypes) {
            types.emplace_back(t.get());
            frameSizeDelta += align(t->size(), kWordSize) / kWordSize;
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
          for (size_t i = 0, n = returnTypes.length(); i < n; i++) {
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
          break;

        default:
          throw ValidateError("", name.str(), buildString("unknown opcode at offset ", inst - insts.begin()));
      }
    }
  }

  // Make sure there's no dead space inside the function.
  size_t prevEnd = 0;
  for (auto& b : blocks) {
    if (b.begin != prevEnd) {
      throw ValidateError(
          "", name.str(),
          buildString("block starting at ", b.begin, " does not start immediately after previous block"));
    }
    prevEnd = b.end;
  }

  // Ensure safepoints are correct.
  auto validSafepoints = buildSafepoints(package);
  if (**validSafepoints != safepoints) {
    throw ValidateError(
      "", name.str(), "invalid safepoints");
  }
}

void Safepoints::init(uint16_t frameSize, BoundArray<uint8_t>& data) {
  data_.init(data.array(), data.length());
  frameSize_ = frameSize;
}

uint32_t Safepoints::lookup(uint32_t instOffset) const {
  ASSERT(data_.length() / bytesPerEntry());
  uint32_t begin = 0;
  auto end = static_cast<uint32_t>(data_.length() / bytesPerEntry());
  while (begin < end) {
    auto mid = (end - begin) / 2;
    auto entry = at(mid);
    if (entry->instOffset == instOffset) {
      return mid;
    } else if (entry->instOffset < instOffset) {
      begin = mid + 1;
    } else {
      end = mid;
    }
  }
  // Caller must not look up safepoint that doesn't exist.
  UNREACHABLE();
  return 0;
}

bool Safepoints::isPointer(uint32_t index, uint16_t slot) const {
  auto entry = at(index);
  auto byteIndex = slot / 8;
  auto bitIndex = slot % 8;
  return (entry->bits[byteIndex] & bitIndex) != 0;
}

uint32_t Safepoints::length() const {
  return data_.length() / bytesPerEntry();
}

size_t Safepoints::bytesPerEntry(uint16_t frameSize) {
  return sizeof(uint32_t) + align(align(frameSize, 8) / 8, sizeof(uint32_t));
}

bool Safepoints::operator == (const Safepoints& that) const {
  if (frameSize_ != that.frameSize_ || data_.length() != that.data_.length()) {
    return false;
  }
  return std::equal(data_.begin(), data_.end(), that.data_.begin());
}

const Safepoints::Entry* Safepoints::at(uint32_t index) const {
  auto offset = static_cast<size_t>(index) * bytesPerEntry();
  return reinterpret_cast<const Entry*>(&data_[offset]);
}

void SafepointBuilder::newEntry(uint32_t instOffset) {
  entries_.emplace_back(Entry{instOffset});
}

void SafepointBuilder::setPointer(uint16_t slot) {
  entries_.back().slots.push_back(slot);
}

Handle<Safepoints> SafepointBuilder::build(uint16_t frameSize) {
  auto safepoints = handle(new (heap->allocate(sizeof(Safepoints))) Safepoints);
  build(frameSize, safepoints);
  return safepoints;
}

void SafepointBuilder::build(uint16_t frameSize, Handle<Safepoints>& safepoints) {
  std::sort(entries_.begin(), entries_.end(), [](const Entry& e1, const Entry& e2) {
    return e1.instOffset < e2.instOffset;
  });
  auto bytesPerEntry = sizeof(uint32_t) + align(align(frameSize, 8) / 8, sizeof(uint32_t));
  auto size = entries_.size() * bytesPerEntry;
  auto data = handle(new(heap->allocate(sizeof(BoundArray<uint8_t>))) BoundArray<uint8_t>());
  data->init(Array<uint8_t>::make(size), size);

  // TODO: check little-endian or support big-endian.
  for (size_t i = 0; i < entries_.size(); i++) {
    auto& e = entries_[i];
    auto p = data->begin() + i * bytesPerEntry;
    *reinterpret_cast<uint32_t*>(p) = e.instOffset;
    for (auto slot : e.slots) {
      auto byteIndex = slot / 8;
      auto bitIndex = slot % 8;
      p[sizeof(uint32_t) + byteIndex] |= 1 << bitIndex;
    }
  }

  safepoints->init(frameSize, **data);
}

}  // namespace codeswitch
