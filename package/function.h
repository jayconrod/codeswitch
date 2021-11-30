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

class Function {
 public:
  Function() = default;
  Function(const String& name, List<Ptr<Type>>& paramTypes, List<Ptr<Type>>& returnTypes, List<Inst>& insts,
           length_t frameSize) :
      name(name), paramTypes(paramTypes), returnTypes(returnTypes), insts(insts), frameSize(frameSize) {}
  static Function* make(const String& name, List<Ptr<Type>>& paramTypes, List<Ptr<Type>>& returnTypes,
                        List<Inst>& insts, length_t frameSize) {
    return new (heap->allocate(sizeof(Function))) Function(name, paramTypes, returnTypes, insts, frameSize);
  }

  void validate(Handle<Package>& package);

  String name;
  List<Ptr<Type>> paramTypes;
  List<Ptr<Type>> returnTypes;
  List<Inst> insts;
  length_t frameSize = 0;
};

}  // namespace codeswitch

#endif
