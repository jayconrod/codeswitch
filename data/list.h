// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef data_list_h
#define data_list_h

#include "array.h"
#include "common/common.h"
#include "memory/handle.h"
#include "memory/ptr.h"

namespace codeswitch {

/**
 * A variable length, contiguous sequence of elements of the same type.
 *
 * List uses Array as a backing store. List keeps track of the size of the
 * Array (cap) and the number of elements in use (length). If the client
 * attempts to append more than cap elements, the existing elements are
 * moved to a new, larger array first. As a result, append takes amortized
 * O(1) time.
 */
template <class T>
class List {
 public:
  List() = default;
  static List* make();
  static Handle<List> create(size_t cap);

  const T& at(size_t i) const { return (*this)[i]; }
  T& at(size_t i) { return (*this)[i]; }
  const T& operator[](size_t i) const { return (*const_cast<List<T>*>(this))[i]; }
  T& operator[](size_t i);
  size_t length() const { return length_; }
  size_t cap() const { return data_.length(); }
  bool empty() const { return length() == 0; }
  template <class S>
  void append(const S& elem);
  template <class S>
  void append(S* elems, size_t n);
  void resize(size_t newLength);
  void reserve(size_t newCap);

  const T* begin() const { return const_cast<List<T>*>(this)->begin(); }
  T* begin() { return data_.begin(); }
  const T* end() const { return const_cast<List<T>*>(this)->end(); }
  T* end() { return data_.begin() + length_; }

 private:
  List(BoundArray<T>* array, size_t length);
  void reserveMore(size_t more);

  BoundArray<T> data_;
  size_t length_ = 0;
};

template <class T>
List<T>::List(BoundArray<T>* data, size_t cap) : data_(data), length_(length) {
  if (length_ > data->length_) {
    throw BoundsCheckError();
  }
}

template <class T>
List<T>* List<T>::make() {
  return new (heap->allocate(sizeof(List))) List();
}

template <class T>
T& List<T>::operator[](size_t i) {
  if (i >= length_) {
    throw BoundsCheckError();
  }
  return data_[i];
}

template <class T>
Handle<List<T>> List<T>::create(size_t length) {
  auto list = handle(List<T>::make());
  list->reserve(length);
  return list;
}

template <class T>
template <class S>
void List<T>::append(const S& elem) {
  reserveMore(1);
  data_[length_++] = elem;
}

template <class T>
template <class S>
void List<T>::append(S* elems, size_t n) {
  reserveMore(n);
  for (size_t i = 0; i < n; i++) {
    data_[length_++] = elems[i];
  }
}

template <class T>
void List<T>::resize(size_t newLength) {
  for (size_t i = newLength; i < length_; i++) {
    data_[i] = T();
  }
  reserve(newLength);
  length_ = newLength;
}

template <class T>
void List<T>::reserve(size_t newCap) {
  if (newCap <= cap()) {
    return;
  }
  auto newArray = Array<T>::make(newCap);
  for (size_t i = 0; i < length_; i++) {
    newArray->at(i) = data_[i];
  }
  data_.set(newArray, newCap);
}

template <class T>
void List<T>::reserveMore(size_t more) {
  if (length() + more <= cap()) {
    return;
  }
  auto newCap = nextPowerOf2(length() + more);
  if (newCap < 8) {
    newCap = 8;
  }
  reserve(newCap);
}

}  // namespace codeswitch

#endif
