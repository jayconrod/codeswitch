// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "handle.h"

#include "chunk.h"

#include "vm/vm.h"

using std::lock_guard;
using std::mutex;

namespace codeswitch {
namespace internal {

Block** LocalHandleStorage::create(Block* block) {
  slots_.push_back(block);
  return &slots_.back();
}

PersistentHandleStorage* PersistentHandleStorage::fromBlock(Block* block) {
  return &Chunk::fromAddress(block)->vm()->handleStorage()->persistent_;
}

Block** PersistentHandleStorage::create(Block* block) {
  lock_guard<mutex> lock(mut_);
  if (free_ != nullptr) {
    auto slot = &free_->block;
    free_ = free_->next();
    *slot = block;
    return slot;
  } else {
    slots_.push_back({.block = block});
    return &slots_.back().block;
  }
}

void PersistentHandleStorage::release(Block** slot) {
  lock_guard<mutex> lock(mut_);
  auto freeSlot = reinterpret_cast<Slot*>(slot);
  freeSlot->setNext(free_);
  free_ = freeSlot;
}

PersistentHandleStorage::Slot* PersistentHandleStorage::Slot::next() {
  return reinterpret_cast<Slot*>(reinterpret_cast<address>(taggedNext) & ~1);
}

void PersistentHandleStorage::Slot::setNext(Slot* next) {
  taggedNext = reinterpret_cast<Slot*>(reinterpret_cast<address>(next) | 1);
}

void HandleStorage::enter() {
  ASSERT(LocalHandleStorage::current_ == nullptr);
  LocalHandleStorage::current_ = new LocalHandleStorage;
  lock_guard<mutex> lock(mut_);
  local_.insert(LocalHandleStorage::current_);
}

void HandleStorage::leave() {
  ASSERT(LocalHandleStorage::current_ != nullptr);
  {
    lock_guard<mutex> lock(mut_);
    local_.erase(LocalHandleStorage::current_);
  }
  delete LocalHandleStorage::current_;
  LocalHandleStorage::current_ = nullptr;
}

thread_local LocalHandleStorage* LocalHandleStorage::current_ = nullptr;

HandleScope::HandleScope() : begin_(LocalHandleStorage::current_->slots_.end()) {
  current_ = this;
  LocalHandleStorage::current_->slots_.push_back(nullptr);  // escape slot
}

HandleScope::~HandleScope() {
  auto from = begin_;
  if (*from != nullptr) {
    from++;
  }
  LocalHandleStorage::current_->slots_.erase(from, LocalHandleStorage::current_->slots_.end());
}

Block** HandleScope::escape(Block* block) {
  ASSERT(*begin_ == nullptr);
  *begin_ = block;
  return &*begin_;
}

thread_local HandleScope* HandleScope::current_ = nullptr;

}  // namespace internal
}  // namespace codeswitch
