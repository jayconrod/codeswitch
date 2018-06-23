// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef roots_h
#define roots_h

namespace codeswitch {
namespace internal {

class Heap;
class Meta;

#define ROOT_LIST(V) V(Meta*, metaMeta)

class Roots {
 public:
  explicit Roots(Heap* heap);

#define GETTER(type, name) \
  type name() const { return name##_; }
  ROOT_LIST(GETTER)
#undef GETTER

 private:
#define FIELD(type, name) type name##_;
  ROOT_LIST(FIELD)
#undef FIELD
};

}  // namespace internal
}  // namespace codeswitch

#endif
