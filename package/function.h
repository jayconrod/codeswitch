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

class Function {
 public:
  Function(const String& name, List<Ptr<Type>>& returnTypes, List<Ptr<Type>>& paramTypes, List<Inst>& insts,
           length_t frameSize) :
      name_(name), returnTypes_(returnTypes), paramTypes_(paramTypes), insts_(insts), frameSize_(frameSize) {}
  static Function* make(const String& name, List<Ptr<Type>>& returnTypes, List<Ptr<Type>>& paramTypes,
                        List<Inst>& insts, length_t frameSize) {
    return new (heap.allocate(sizeof(Function))) Function(name, returnTypes, paramTypes, insts, frameSize);
  }

  const String& name() const { return name_; }
  const List<Ptr<Type>>& returnTypes() const { return returnTypes_; }
  const List<Ptr<Type>>& paramTypes() const { return paramTypes_; }
  const List<Inst>& insts() const { return insts_; }
  length_t frameSize() const { return frameSize_; }

 private:
  String name_;
  List<Ptr<Type>> returnTypes_;
  List<Ptr<Type>> paramTypes_;
  List<Inst> insts_;
  length_t frameSize_;
};

}  // namespace codeswitch

#endif
