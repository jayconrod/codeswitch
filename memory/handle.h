// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef handle_h
#define handle_h

#include <deque>
#include <mutex>
#include <unordered_set>
#include "common/common.h"

namespace codeswitch {
namespace internal {

class Block;
class VM;

/**
 * Handle tracks the location of an object on the CodeSwitch heap.
 *
 * Handle stores CodeSwitch pointers in C++ code. Handles serve two purposes. First, they allow
 * C++ code to track addresses of objects that may be moved by the garbage collector. Second,
 * they allow the garbage collector to track pointers into the heap; the garbage collector
 * won't recycle objects reachable from live handles.
 *
 * Handle is a base class. It should not be instantiated directly. Subclasses are responsible
 * for allocating and destroying handle slots.
 */
template <class T>
class Handle {
 public:
  T* operator*() const;
  T* operator->() const;
  T* getOrNull() const;

  bool isEmpty() const;

 protected:
  Handle() = default;
  explicit Handle(T** slot) : slot_(slot) {}

  T** slot_ = nullptr;
};

/**
 * Local is a Handle subclass optimized for stack lifetime variables.
 *
 * Local slots are allocated from LocalHandleStorage. The storage itself is
 * accessed through thread local storage without locking, and new slots are
 * allocated by appending to a deque, so allocation is quite fast. However, this
 * means locals can only be used inside VM code when TLS is set. Storage can be
 * reclaimed using HandleScope, which erases the end of the deque.
 */
template <class T>
class Local : public Handle<T> {
 public:
  Local() = default;
  template <class S>
  explicit Local(S* block);
  Local(const Local& local);
  template <class S>
  Local(const Handle<S>& handle);
  Local& operator=(const Local& local);
  template <class S>
  Local& operator=(const Handle<S>& local);

 private:
  template <class S>
  friend class Local;
};

template <class T>
Local<T> handle(T* value) {
  return Local<T>(value);
}

/**
 * Persistent is a Handle subclass optimized for longer-lived
 * (non-stack-lifetime) variables.
 *
 * Persistent slots are allocated from PersistentHandleStorage. Null pointers
 * cannot be stored, since the (non-null) pointers are used to look up the VM
 * and the storage.  Creating and deleting persistent slots requires locking, so
 * this is expected to be slower than Local. However, Persistent can be used
 * outside the VM, and it's not restricted to stack lifetime.
 */
template <class T>
class Persistent : public Handle<T> {
 public:
  Persistent() = default;
  template <class S>
  explicit Persistent(S* block);
  Persistent(const Persistent& persistent);
  template <class S>
  Persistent(const Handle<S>& handle);
  Persistent(Persistent&& persistent);
  Persistent& operator=(const Persistent& persistent);
  template <class S>
  Persistent& operator=(const Handle<S>& handle);
  Persistent& operator=(Persistent&& persistent);
  ~Persistent();

 private:
  template <class S>
  friend class Persistent;
};

class LocalHandleStorage {
 public:
  NON_COPYABLE(LocalHandleStorage)
  LocalHandleStorage() = default;

  Block** create(Block* block);
  static LocalHandleStorage* current() { return current_; }

 private:
  std::deque<Block*> slots_;
  static thread_local LocalHandleStorage* current_;
  friend class HandleScope;
  friend class HandleStorage;
  friend class VM;
};

class PersistentHandleStorage {
 public:
  NON_COPYABLE(PersistentHandleStorage)
  PersistentHandleStorage() = default;

  static PersistentHandleStorage* fromBlock(Block* block);

  Block** create(Block* block);
  void release(Block** slot);

 private:
  union Slot {
    Block* block;
    Slot* taggedNext;
    Slot* next();
    void setNext(Slot* next);
  };

  std::mutex mut_;
  std::deque<Slot> slots_;
  Slot* free_ = {0};
};

class HandleStorage {
 public:
  NON_COPYABLE(HandleStorage)
  HandleStorage() = default;

  void enter();
  void leave();

 private:
  PersistentHandleStorage persistent_;
  std::mutex mut_;
  std::unordered_set<LocalHandleStorage*> local_;
  friend class PersistentHandleStorage;
};

class HandleScope {
 public:
  NON_COPYABLE(HandleScope)
  HandleScope();
  ~HandleScope();

 private:
  Block** escape(Block* block);

  static thread_local HandleScope* current_;
  std::deque<Block*>::iterator begin_;
};

template <class T>
T* Handle<T>::operator*() const {
  ASSERT(!isEmpty());
  return *slot_;
}

template <class T>
T* Handle<T>::operator->() const {
  ASSERT(!isEmpty());
  return *slot_;
}

template <class T>
T* Handle<T>::getOrNull() const {
  return isEmpty() ? nullptr : *slot_;
}

template <class T>
bool Handle<T>::isEmpty() const {
  return slot_ == nullptr;
}

template <class T>
template <class S>
Local<T>::Local(S* block) : Handle<T>(reinterpret_cast<T**>(LocalHandleStorage::current()->create(block))) {
  CHECK_SUBTYPE_VALUE(T*, block);
}

template <class T>
Local<T>::Local(const Local& local) {
  if (!local.isEmpty()) {
    this->slot_ = reinterpret_cast<T**>(LocalHandleStorage::current()->create(*local.slot_));
  }
}

template <class T>
template <class S>
Local<T>::Local(const Handle<S>& handle) {
  if (!handle.isEmpty()) {
    CHECK_SUBTYPE_VALUE(T*, *handle.slot_);
    this->slot_ = reinterpret_cast<T**>(LocalHandleStorage::current()->create(*handle.slot_));
  }
}

template <class T>
Local<T>& Local<T>::operator=(const Local<T>& local) {
  if (this->slot_ == nullptr && local.slot_ == nullptr) {
    // no assignment needed
  } else if (this->slot_ == nullptr) {
    this->slot_ = reinterpret_cast<T**>(LocalHandleStorage::current()->create(*local.slot_));
  } else if (local.slot_ == nullptr) {
    *this->slot_ = nullptr;
    this->slot_ = nullptr;
  } else {
    *this->slot_ = *local.slot_;
  }
  return *this;
}

template <class T>
template <class S>
Local<T>& Local<T>::operator=(const Handle<S>& handle) {
  if (this->slot_ == nullptr && handle.slot_ == nullptr) {
    // no assignment needed
  } else if (this->slot_ == nullptr) {
    this->slot_ = reinterpret_cast<T**>(LocalHandleStorage::current()->create(*handle.slot_));
  } else if (handle.slot_ == nullptr) {
    *this->slot_ = nullptr;
    this->slot_ = nullptr;
  } else {
    *this->slot_ = *handle.slot_;
  }
  CHECK_SUBTYPE_VALUE(T*, *handle.slot_);
  return *this;
}

template <class T>
template <class S>
Persistent<T>::Persistent(S* block) :
    Handle<T>(reinterpret_cast<T**>(PersistentHandleStorage::fromBlock(block)->create(block))) {
  CHECK_SUBTYPE_VALUE(T*, block);
}

template <class T>
Persistent<T>::Persistent(const Persistent<T>& persistent) {
  if (!persistent.isEmpty()) {
    this->slot_ =
        reinterpret_cast<T**>(PersistentHandleStorage::fromBlock(*persistent.slot_)->create(*persistent.slot_));
  }
}

template <class T>
template <class S>
Persistent<T>::Persistent(const Handle<S>& handle) {
  if (!handle.isEmpty()) {
    CHECK_SUBTYPE_VALUE(T*, *handle.slot_);
    ASSERT(handle.slot_ != nullptr);
    this->slot_ = reinterpret_cast<T**>(PersistentHandleStorage::fromBlock(*handle.slot_)->create(*handle.slot_));
  }
}

template <class T>
Persistent<T>::Persistent(Persistent<T>&& persistent) : Handle<T>(persistent.slot_) {
  persistent.slot_ = nullptr;
}

template <class T>
Persistent<T>& Persistent<T>::operator=(const Persistent<T>& persistent) {
  if (this->slot_ == nullptr && persistent.slot_ == nullptr) {
    // no assignment needed
  } else if (this->slot_ == nullptr) {
    this->slot_ =
        reinterpret_cast<T**>(PersistentHandleStorage::fromBlock(*persistent.slot_)->create(*persistent.slot_));
  } else if (persistent.slot_ == nullptr) {
    *this->slot_ = nullptr;
    this->slot_ = nullptr;
  } else {
    *this->slot_ = *persistent.slot_;
  }
  return *this;
}

template <class T>
template <class S>
Persistent<T>& Persistent<T>::operator=(const Handle<S>& handle) {
  if (this->slot_ == nullptr && handle.slot_ == nullptr) {
    // no assignment needed
  } else if (this->slot_ == nullptr) {
    ASSERT(*handle.slot_ != nullptr);
    this->slot_ = reinterpret_cast<T**>(PersistentHandleStorage::fromBlock(*handle.slot_)->create(*handle.slot_));
  } else if (handle.slot_ == nullptr) {
    *this->slot_ = nullptr;
    this->slot_ = nullptr;
  } else {
    *this->slot_ = *handle.slot_;
  }
  CHECK_SUBTYPE_VALUE(T*, *handle.slot_);
  return *this;
}

template <class T>
Persistent<T>& Persistent<T>::operator=(Persistent<T>&& persistent) {
  if (this->slot_ != nullptr) {
    PersistentHandleStorage::fromBlock(*this->slot_)->release(this->slot_);
  }
  this->slot_ = persistent.slot_;
  persistent.slot_ = nullptr;
  return *this;
}

template <class T>
Persistent<T>::~Persistent() {
  if (this->slot_ != nullptr) {
    PersistentHandleStorage::fromBlock(*this->slot_)->release(this->slot_);
  }
}

}  // namespace internal
}  // namespace codeswitch

#endif
