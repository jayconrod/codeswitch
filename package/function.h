// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef package_function_h
#define package_function_h

#include "data/list.h"
#include "data/string.h"
#include "inst.h"
#include "memory/handle.h"
#include "memory/heap.h"
#include "memory/ptr.h"
#include "type.h"

namespace codeswitch {

class Package;

/**
 * A set of bitmaps indicating which words in a stack from contain pointers
 * to the heap. Each bitmap corresponds to a point in a function where the
 * garbage collector may be active (for example, an allocation or a call site).
 * All bitmaps are the size of the maximum frame size, aligned to 8 words.
 * If the frame isn't actually that big, excess bits should indicate no pointer
 * is present.
 */
class Safepoints {
 public:
  Safepoints() = default;
  Safepoints(uint16_t frameSize, BoundArray<uint8_t>& data) { init(frameSize, data); }
  Safepoints& operator = (const Safepoints&) = default;
  void init(uint16_t frameSize, BoundArray<uint8_t>& data);

  /**
   * Given the offset of an instruction with the function, lookup returns
   * the index of the corresponding safepoint. This index may be used with
   * isPointer.
   */
  uint32_t lookup(uint32_t instOffset) const;

  /**
   * For the safepoint at index (retrieved with lookup), indicates whether
   * the given stack slot contains a pointer.
   */
  bool isPointer(uint32_t index, uint16_t slot) const;

  /**
   * The maximum size of the function's stack frame in words. This includes
   * locals, temporaries, and arguments to other functions. It does not
   * include arguments to the function itself or control words.
   */
  uint16_t frameSize() const { return frameSize_; }

  uint32_t length() const;
  size_t bytesPerEntry() const { return bytesPerEntry(frameSize_); }
  static size_t bytesPerEntry(uint16_t frameSize);
  const BoundArray<uint8_t>& data() const { return data_; }

  bool operator == (const Safepoints& that) const;
  bool operator != (const Safepoints& that) const { return !(*this == that); }

 private:
  struct Entry {
    uint32_t instOffset;
    uint8_t bits[0];
  };

  const Entry* at(uint32_t index) const;

  BoundArray<uint8_t> data_;
  uint16_t frameSize_ = 0;
};

class SafepointBuilder {
 public:
  void newEntry(uint32_t instOffset);
  void setPointer(uint16_t slot);
  Handle<Safepoints> build(uint16_t frameSize);
  void build(uint16_t frameSize, Handle<Safepoints>& safepoints);

 private:
  struct Entry {
    uint32_t instOffset = 0;
    std::vector<uint16_t> slots;
  };

  std::vector<Entry> entries_;
};

class Function {
 public:
  Function() = default;
  Function(const String& name, List<Ptr<Type>>& paramTypes, List<Ptr<Type>>& returnTypes, List<Inst>& insts,
           const Safepoints& safepoints) :
      name(name), paramTypes(paramTypes), returnTypes(returnTypes), insts(insts), safepoints(safepoints) {}
  static Function* make(const String& name, List<Ptr<Type>>& paramTypes, List<Ptr<Type>>& returnTypes,
                        List<Inst>& insts, const Safepoints& safepoints) {
    return new (heap->allocate(sizeof(Function))) Function(name, paramTypes, returnTypes, insts, safepoints);
  }

  Handle<Safepoints> buildSafepoints(Handle<Package>& package);
  void validate(Handle<Package>& package);

  String name;
  // TODO: these should be BoundArrays.
  List<Ptr<Type>> paramTypes;
  List<Ptr<Type>> returnTypes;
  List<Inst> insts;
  Safepoints safepoints;
};

}  // namespace codeswitch

#endif
