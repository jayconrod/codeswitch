// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_asm_h
#define package_asm_h

#include <cstdint>
#include <filesystem>
#include <string>
#include "data/array.h"
#include "inst.h"
#include "memory/handle.h"
#include "package.h"

namespace codeswitch {

Handle<Package> readPackageAsm(const std::filesystem::path& filename, std::istream& is);
void writePackageAsm(std::ostream& os, Package* package);

/**
 * Tracks offsets within a function, used for assembling instructions that need
 * to reference other instructions, particularly branches.
 *
 * If a label is bound (Assembler::bind has been called on it), the label
 * refers to an earlier instruction. Later references to it will use negative
 * offsets. If a label is unbound, it refers to an instruction that hasn't been
 * assembled yet. When the label is bound, all references are updated to
 * a positive offset.
 */
class Label {
 public:
  bool bound() const { return bound_; }

 private:
  friend class Assembler;

  int32_t offset_ = 0;
  bool bound_ = false;
};

class Assembler {
 public:
  Assembler();
  Handle<List<Inst>> finish();

  void bind(Label* label);

  void add();
  void and_();
  void asr();
  void b(Label* label);
  void bif(Label* label);
  void call(uint32_t index);
  void div();
  void eq();
  void false_();
  void ge();
  void gt();
  void int64(int64_t n);
  void le();
  void loadarg(uint16_t n);
  void loadlocal(uint16_t n);
  void lt();
  void mod();
  void mul();
  void ne();
  void neg();
  void nop();
  void not_();
  void or_();
  void ret();
  void shl();
  void shr();
  void storearg(uint16_t n);
  void storelocal(uint16_t n);
  void sub();
  void sys(Sys sys);
  void true_();
  void unit();
  void xor_();

 private:
  void ensureSpace(size_t n);
  void op(Op op);
  void op1_8(Op op, uint8_t a);
  void op1_16(Op op, uint16_t a);
  void op1_32(Op op, uint32_t a);
  void op1_64(Op op, uint64_t a);
  void op1_label(Op op, Label* label);
  uint8_t* addrOf(int32_t offset);

  struct Fragment {
    template <class T>
    void push(T n) {
      // TODO: reverse bytes on big-endian systems
      *reinterpret_cast<T*>(end) = n;
      end += sizeof(T);
    }
    uint8_t* end = begin;
    uint8_t begin[4088] = {0};
  };

  std::deque<Fragment> fragments_;
  size_t size_ = 0;
};

}  // namespace codeswitch

#endif
