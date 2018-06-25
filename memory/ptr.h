// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef ptr_h
#define ptr_h

#include "common/common.h"

namespace codeswitch {
namespace internal {

/**
 * Wrapper for pointers stored within Blocks. Ensures that pointer fields are
 * properly initialized and writes are recorded.
 */
template <class T>
class alignas(word_t) Ptr {
 public:
  Ptr() : p_(nullptr) {}
  explicit Ptr(T* q) { set(q); }
  NON_COPYABLE(Ptr)

  T* get() const { return p_; }
  void set(T* q) {
    p_ = q;
    Heap::recordWrite(&p_, q);
  }

  bool operator==(T* q) { return p_ == q; }
  template <class S>
  bool operator==(const Ptr<S>& q) const {
    return p_ == q.p_;
  }
  bool operator!=(T* q) const { return p_ != q; }
  template <class S>
  bool operator!=(const Ptr<S>& q) const {
    return p_ != q.p_;
  }
  operator bool() const { return p_ != nullptr; }
  bool operator!() const { return p_ == nullptr; }

 private:
  T* p_;
};

}  // namespace internal
}  // namespace codeswitch

#endif
