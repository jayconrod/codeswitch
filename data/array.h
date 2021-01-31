// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef data_array_h
#define data_array_h

#include <cstdint>

#include "common/common.h"
#include "memory/heap.h"

namespace codeswitch {
namespace internal {

/** A fixed-length contiguous range of elements. */
template <class T>
class Array {
 public:
  static Array* alloc(length_t length);
  Array() {}
  NON_COPYABLE(Array)

  const T& at(length_t i) const { return (*this)[i]; }
  T& at(length_t i) { return (*this)[i]; }
  const T& operator[](length_t i) const { return data_[i]; }
  T& operator[](length_t i) { return data_[i]; }
  const Array* slice(length_t i) const;
  Array* slice(length_t i);

 private:
  T data_[0];
};

template <class T>
Array<T>* Array<T>::alloc(length_t length) {
  return new (heap.allocate(length * sizeof(T))) Array<T>;
}

template <class T>
const Array<T>* Array<T>::slice(length_t i) const {
  return reinterpret_cast<const Array<T>*>(&data_[i]);
}

template <class T>
Array<T>* Array<T>::slice(length_t i) {
  return reinterpret_cast<Array<T>*>(&data_[i]);
}

}  // namespace internal
}  // namespace codeswitch

#endif
