// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef interpreter_interpreter_h
#define interpreter_interpreter_h

namespace codeswitch {
namespace internal {

template <class T>
class Handle;
class Function;

void interpret(Handle<Function> f);

}  // namespace internal
}  // namespace codeswitch

#endif
