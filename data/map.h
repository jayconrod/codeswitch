// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef map_h
#define map_h

#include "array.h"
#include "common/common.h"
#include "memory/ptr.h"

namespace codeswitch {

template <class K>
class HashRef {
  static word_t hash(const K& key) { return key->hash(); }
  static bool equal(const K& l, const K& r) { return *l == *r; }
};

template <class K, class V, class H>
class Map {
 public:
  static Map* make();

  bool empty() const { return length() == 0; }
  length_t length() const { return length_; }
  length_t cap() const { return cap_; }
  bool has(const K& key) const;
  const V& get(const K& key) const { return const_cast<Map<K, V, H>*>(this)->get(key); }
  V& get(const K& key);
  void set(const K& key, const V& value);

 protected:
  static const word_t kMinCap = 16;
  static const word_t kUnused = 0;
  struct Entry {
    word_t hash;  // in use if non-zero
    K key;
    V value;
  };

  word_t mask() const { return cap_ - 1; }
  static word_t hash(const K& key);
  void resize(length_t newCap);
  const Entry* find(const K& key, word_t h) const { return const_cast<Map<K, V, H>*>(this)->find(key, h); }
  Entry* find(const K& key, word_t h);
  Entry* findOrAdd(const K& key, word_t h);

  Ptr<Array<Entry>> data_;
  length_t length_ = 0;
  length_t cap_ = 0;  // number of entries; 0 or power of 2
};

template <class K, class V, class H>
Map<K, V, H>* Map<K, V, H>::make() {
  return new (heap->allocate(sizeof(Map<K, V, H>))) Map<K, V, H>;
}

template <class K, class V, class H>
bool Map<K, V, H>::has(const K& key) const {
  return this->find(key, hash(key)) != nullptr;
}

template <class K, class V, class H>
V& Map<K, V, H>::get(const K& key) {
  auto e = this->find(key, hash(key));
  ASSERT(e != nullptr);
  return e->value;
}

template <class K, class V, class H>
void Map<K, V, H>::set(const K& key, const V& value) {
  auto e = this->findOrAdd(key, hash(key));
  e->value = value;
}

template <class K, class V, class H>
void Map<K, V, H>::resize(length_t newCap) {
  ASSERT(newCap >= length_);
  ASSERT(isPowerOf2(newCap));
  if (newCap == 0) {
    data_.set(nullptr);
    return;
  }

  auto newData = Array<Entry>::make(newCap);
  auto oldData = data_.get();
  auto oldCap = cap_;
  data_.set(newData);
  length_ = 0;
  cap_ = newCap;

  for (length_t i = 0; i < oldCap; i++) {
    auto oldE = &oldData->at(i);
    if (oldE->hash != kUnused) {
      auto e = this->findOrAdd(oldE->key, oldE->hash);
      e->value = std::move(oldE->value);
    }
  }
}

template <class K, class V, class H>
typename Map<K, V, H>::Entry* Map<K, V, H>::find(const K& key, word_t h) {
  if (length_ == 0) {
    return nullptr;
  }
  auto start = h & mask();
  for (word_t i = 0; i < cap_; i++) {
    auto e = &data_->at((start + i) & mask());
    if (e->hash == kUnused) {
      return nullptr;
    } else if (e->hash == h && H::equal(e->key, key)) {
      return e;
    }
  }
  return nullptr;
}

template <class K, class V, class H>
typename Map<K, V, H>::Entry* Map<K, V, H>::findOrAdd(const K& key, word_t h) {
  if (cap_ == 0) {
    resize(kMinCap);
  }

  auto start = h & mask();
  for (word_t i = 0; i < cap_; i++) {
    auto e = &data_->at((start + i) & mask());
    if (e->hash == kUnused) {
      if (length_ * 2 < cap_) {
        length_++;
        e->hash = h;
        e->key = key;
        return e;
      } else {
        break;
      }
    } else if (e->hash == h && H::equal(e->key, key)) {
      return e;
    }
  }
  resize(cap_ * 2);
  return findOrAdd(key, h);
}

template <class K, class V, class H>
word_t Map<K, V, H>::hash(const K& key) {
  auto h = H::hash(key);
  if (h == Map<K, V, H>::kUnused) {
    h++;
  }
  return h;
}

}  // namespace codeswitch

#endif
