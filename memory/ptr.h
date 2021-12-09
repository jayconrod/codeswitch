// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_ptr_h
#define memory_ptr_h

#include <functional>
#include "common/common.h"
#include "heap.h"

namespace codeswitch {

/**
 * Wrapper for pointers stored within Blocks. Ensures that pointer fields are
 * properly initialized and writes are recorded. Generally should not be used
 * as a value, since it doesn't make sense to record a write to the C++ stack.
 */
template <class T>
class alignas(uintptr_t) Ptr {
 public:
  Ptr() : p_(nullptr) {}
  template <class S>
  Ptr(S* q) {
    set(q);
  }
  template <class S>
  Ptr(const Ptr<S>& q) {
    set(q.p_);
  }
  template <class S>
  Ptr(Ptr<S>&& q) {
    set(q.p_);
    q.set(nullptr);
  }
  template <class S>
  Ptr& operator=(const Ptr<S>& q) {
    set(q.p_);
    return *this;
  }
  template <class S>
  Ptr& operator=(Ptr<S>&& q) {
    set(q.p_);
    q.set(nullptr);
    return *this;
  }

  const T* get() const { return p_; }
  T* get() { return p_; }
  const T& operator*() const { return *p_; }
  T& operator*() { return *p_; }
  const T* operator->() const { return p_; }
  T* operator->() { return p_; }
  void set(T* q) {
    p_ = q;
    heap->recordWrite(&p_, q);
  }

  template <class S>
  bool operator==(const Ptr<S>& q) const {
    return p_ == q.p_;
  }
  template <class S>
  bool operator!=(const Ptr<S>& q) const {
    return p_ != q.p_;
  }
  operator bool() const { return p_ != nullptr; }
  bool operator!() const { return p_ == nullptr; }

 private:
  T* p_;
};

template <class T>
class PtrHash {
 public:
  static uintptr_t hash(const Ptr<T>& p) { return std::hash(reinterpret_cast<uintptr_t>(p.get())); }
  static bool equal(const Ptr<T>& l, const Ptr<T>& r) { return l == r; }
};

}  // namespace codeswitch

#endif
