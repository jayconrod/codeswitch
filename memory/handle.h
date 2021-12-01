// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef memory_handle_h
#define memory_handle_h

#include <deque>
#include <mutex>
#include "common/common.h"
#include "ptr.h"

namespace codeswitch {

/**
 * Handle tracks a reference to a block on the heap from outside the heap,
 * for example, from a local variable on the C++ stack.
 */
template <class T>
class Handle {
 public:
  Handle() = default;
  explicit Handle(T* block);
  Handle(const Handle& handle);
  Handle(Handle&& handle);
  ~Handle();
  Handle& operator=(const Handle& handle);
  Handle& operator=(Handle&& handle);

  operator bool() const { return slot_; }
  const T* operator*() const { return **const_cast<Handle<T>*>(this); }
  T* operator*() {
    ASSERT(slot_);
    return *slot_;
  }
  const T* getOrNull() const { return slot_ ? slot_->get() : nullptr; }
  T* getOrNull() { return slot_ ? slot_->get() : nullptr; }
  const T* operator->() const { return **this; }
  T* operator->() { return **this; }
  void reset();

 private:
  T** slot_ = nullptr;
};

template <class T>
Handle<T> handle(T* block) {
  return Handle<T>(block);
}

/**
 * HandleStorage tracks all live handles.
 *
 * Each handle is given a word-sized slot. When allocated, a slot contains a
 * pointer to the tracked block. When free, a slot contains the uintptr_t of
 * another slot on the free list with the low bit set.
 */
class HandleStorage {
 public:
  uintptr_t allocSlot();
  void freeSlot(uintptr_t slot);

 private:
  std::mutex mu_;
  std::deque<uintptr_t> slots_;
  uintptr_t free_ = 0;
};

extern HandleStorage handleStorage;

template <class T>
Handle<T>::Handle(T* block) : slot_(reinterpret_cast<T**>(handleStorage.allocSlot())) {
  *slot_ = block;
}

template <class T>
Handle<T>::Handle(const Handle<T>& handle) {
  if (handle.slot_ != nullptr) {
    slot_ = reinterpret_cast<Ptr<T>*>(handleStorage.allocSlot());
    *slot_ = *handle.slot_;
  }
}

template <class T>
Handle<T>::Handle(Handle<T>&& handle) : slot_(handle.slot_) {
  handle.slot_ = nullptr;
}

template <class T>
Handle<T>::~Handle() {
  if (slot_ != nullptr) {
    handleStorage.freeSlot(reinterpret_cast<uintptr_t>(slot_));
  }
}

template <class T>
Handle<T>& Handle<T>::operator=(const Handle<T>& handle) {
  if (handle.slot_ == nullptr) {
    if (slot_ != nullptr) {
      handleStorage.freeSlot(reinterpret_cast<uintptr_t>(slot_));
      slot_ = nullptr;
    }
  } else {
    if (slot_ == nullptr) {
      slot_ = reinterpret_cast<T**>(handleStorage.allocSlot());
    }
    *slot_ = *handle.slot_;
  }
  return *this;
}

template <class T>
Handle<T>& Handle<T>::operator=(Handle<T>&& handle) {
  if (slot_ != nullptr) {
    handleStorage.freeSlot(reinterpret_cast<uintptr_t>(slot_));
  }
  slot_ = handle.slot_;
  handle.slot_ = nullptr;
  return *this;
}

template <class T>
void Handle<T>::reset() {
  if (slot_ != nullptr) {
    handleStorage.freeSlot(reinterpret_cast<uintptr_t>(slot_));
  }
  slot_ = nullptr;
}

}  // namespace codeswitch

#endif
