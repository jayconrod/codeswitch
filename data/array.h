// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef data_array_h
#define data_array_h

#include <cstdint>
#include <iterator>

#include "common/common.h"
#include "memory/heap.h"
#include "memory/ptr.h"

namespace codeswitch {

/**
 * A contiguous list of elements. The length is set at allocation time. The
 * array doesn't store the length, so it performs no bounds checking.
 */
template <class T>
class Array {
 public:
  static Array* make(size_t length);
  Array() {}
  NON_COPYABLE(Array)

  const T& at(size_t i) const { return (*this)[i]; }
  T& at(size_t i) { return (*this)[i]; }
  const T& operator[](size_t i) const { return *(begin() + i); }
  T& operator[](size_t i) { return *(begin() + i); }

  const T* begin() const { return const_cast<Array<T>*>(this)->begin(); }
  T* begin() { return reinterpret_cast<T*>(this); }

  const Array<T>* slice(size_t i) const { return const_cast<Array<T>*>(this)->slice(i); }
  Array<T>* slice(size_t i);
};

template <class T>
Array<T>* Array<T>::make(size_t length) {
  return new (heap->allocate(length * sizeof(T))) Array<T>;
}

template <class T>
Array<T>* Array<T>::slice(size_t i) {
  return reinterpret_cast<Array<T>*>(reinterpret_cast<uintptr_t>(this) + i * sizeof(T));
}

template <class T>
class BoundArray {
 public:
  BoundArray() = default;
  BoundArray(Array<T>* array, size_t length) : array_(array), length_(length) {}
  void init(Array<T>* array, size_t length);

  bool isNull() const { return !array_; }
  size_t length() const { return length_; }
  const T& at(size_t i) const { return const_cast<BoundArray<T>*>(this)->at(i); }
  T& at(size_t i) { return (*this)[i]; }
  const T& operator[](size_t i) const { return (*const_cast<BoundArray<T>*>(this))[i]; }
  T& operator[](size_t i);

  const Array<T>* array() const { return const_cast<BoundArray<T>*>(this)->array(); }
  Array<T>* array() { return array_.get(); }
  const T* begin() const { return const_cast<BoundArray<T>*>(this)->begin(); }
  T* begin() { return array_->begin(); }
  const T* end() const { return const_cast<BoundArray<T>*>(this)->end(); }
  T* end() { return array_->begin() + length_; }

  void slice(size_t i, size_t j);
  void set(Array<T>* array, size_t length);

 private:
  Ptr<Array<T>> array_;
  size_t length_ = 0;
};

template <class T>
void BoundArray<T>::init(Array<T>* array, size_t length) {
  array_ = array;
  length_ = length;
}

template <class T>
T& BoundArray<T>::operator[](size_t i) {
  if (i >= length_) {
    throw BoundsCheckError();
  }
  return array_->at(i);
}

template <class T>
void BoundArray<T>::slice(size_t i, size_t j) {
  if (j > length_ || i > j) {
    throw BoundsCheckError();
  }
  array_ = array_->slice(i);
  length_ = j - i;
}

template <class T>
void BoundArray<T>::set(Array<T>* array, size_t length) {
  array_.set(array);
  length_ = length;
}

}  // namespace codeswitch

#endif
